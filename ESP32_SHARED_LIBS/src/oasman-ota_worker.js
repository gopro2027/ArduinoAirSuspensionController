/**
 * OASMan unified OTA worker
 * GET ?firmware=<FIRMWARE_RELEASE_NAME>&tag=<RELEASE_TAG_NAME>
 * - 204 if newest release (for that firmware) tag matches tag (already up to date)
 * - 200 + firmware binary if a newer release is available
 * - blank/missing tag is treated as outdated (serve the newest matching binary)
 * Deploy as: oasman-ota → http://oasman-ota.gopro2027.workers.dev/
 */

/**
 * List releases (newest first) instead of only /latest, so a device can fall back
 * to the newest release that actually ships ITS firmware asset. A hotfix that only
 * rebuilds one variant must not strand every other variant on a 404.
 */
const RELEASES_LIST_URL =
  'https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases?per_page=30';

const BINARY_URL_PREFIX =
  'https://github.com/gopro2027/ArduinoAirSuspensionController/releases/download/';

const FIRMWARE_NAME_RE = /^[a-zA-Z0-9_]+$/;
const CACHE_TTL_MS = 30 * 60 * 1000;
/** Bump when binary cache/response format changes (v1 streamed bodies broke ESP32 HTTPClient). */
const BINARY_CACHE_VERSION = 'v2-buffered';

const FIRMWARE_BIN_SUFFIX = '_firmware.bin';

function binaryCacheRequest(downloadUrl) {
  return new Request(`${downloadUrl}#${BINARY_CACHE_VERSION}`, { method: 'GET' });
}

function listFirmwareBinAssets(release) {
  return (release?.assets || []).filter(
    (a) => a.name?.endsWith(FIRMWARE_BIN_SUFFIX) && a.browser_download_url
  );
}

/** Remove cached firmware binaries across releases (URLs are tag-specific). */
async function invalidateFirmwareBinaries(releases) {
  const cache = caches.default;
  const assets = (releases || []).flatMap(listFirmwareBinAssets);
  await Promise.all(
    assets.map((asset) => cache.delete(binaryCacheRequest(asset.browser_download_url)))
  );
  if (assets.length > 0) {
    console.log(`Invalidated ${assets.length} firmware binary cache entries`);
  }
}

/** Tag of the newest (first) non-draft release in a list, used for cache change detection. */
function newestReleaseTag(releases) {
  const newest = (releases || []).find((r) => !r?.draft);
  return newest?.tag_name || null;
}

async function parseCachedReleases(cachedResponse) {
  if (!cachedResponse) return null;
  try {
    const buffer = await cachedResponse.arrayBuffer();
    const parsed = JSON.parse(new TextDecoder().decode(buffer));
    return Array.isArray(parsed) ? parsed : null;
  } catch {
    return null;
  }
}

function releasesTagFromCache(cachedResponse, releases) {
  return cachedResponse?.headers.get('X-Release-Tag') || newestReleaseTag(releases);
}

async function fetchCachedReleasesJson() {
  const cacheKey = new Request(RELEASES_LIST_URL, { method: 'GET' });
  const cache = caches.default;
  const cachedResponse = await cache.match(cacheKey);
  let cachedReleases = null;
  if (cachedResponse) {
    cachedReleases = await parseCachedReleases(cachedResponse);
    const cacheExpiry = cachedResponse.headers.get('X-Cache-Expiry');
    if (cacheExpiry && Date.now() < parseInt(cacheExpiry, 10) && cachedReleases) {
      return {
        ok: true,
        releases: cachedReleases,
        fromCache: true,
      };
    }
  }

  const response = await fetch(RELEASES_LIST_URL, {
    headers: {
      'User-Agent': 'OASMan-OTA/1.0',
      Accept: 'application/vnd.github+json',
    },
  });

  const buffer = await response.arrayBuffer();
  const now = Date.now();
  const cacheExpiry = now + CACHE_TTL_MS;
  const rateLimitReset = response.headers.get('x-ratelimit-reset');

  if (response.status === 403 || response.status === 429) {
    if (cachedReleases) {
      return {
        ok: true,
        releases: cachedReleases,
        fromCache: true,
        rateLimited: true,
      };
    }
    return {
      ok: false,
      status: 429,
      body: JSON.stringify({
        error: 'Rate limit exceeded and no cached release data available',
        retryAfter: rateLimitReset,
      }),
    };
  }

  if (!response.ok) {
    return {
      ok: false,
      status: response.status,
      body: buffer,
    };
  }

  const parsed = JSON.parse(new TextDecoder().decode(buffer));
  const releases = Array.isArray(parsed) ? parsed : [];
  const newTag = newestReleaseTag(releases);
  const previousTag = releasesTagFromCache(cachedResponse, cachedReleases);

  if (previousTag && newTag && previousTag !== newTag && cachedReleases) {
    console.log(`Release tag changed ${previousTag} -> ${newTag}, clearing firmware caches`);
    await invalidateFirmwareBinaries(cachedReleases);
  }

  const responseToCache = new Response(buffer, {
    status: response.status,
    headers: {
      'Content-Type': 'application/json',
      'X-Cache-Expiry': cacheExpiry.toString(),
      'X-Cache-Time': new Date(now).toISOString(),
      'X-Release-Tag': newTag || '',
      'Cache-Control': 'public, max-age=1800',
    },
  });
  await cache.put(cacheKey, responseToCache.clone());

  return {
    ok: true,
    releases,
    fromCache: false,
  };
}

/**
 * Newest (list is newest-first) non-draft release that ships this device's firmware.
 * This is the fix: a variant-specific hotfix release no longer hides older releases
 * that are still the latest available build for every other variant.
 */
function findReleaseWithFirmware(releases, firmware) {
  const assetName = `${firmware}_firmware.bin`;
  for (const release of releases || []) {
    if (release?.draft) continue;
    const asset = (release.assets || []).find(
      (a) => a.name === assetName && a.browser_download_url
    );
    if (asset) {
      return { release, asset };
    }
  }
  return null;
}

/**
 * Fixed-length firmware body for ESP32 HTTPClient (must not use chunked Transfer-Encoding).
 * Body is a Uint8Array copy so Content-Length always matches bytes on the wire.
 */
function binaryResponse(buffer, extraHeaders = {}) {
  const body = new Uint8Array(buffer);
  const headers = new Headers(extraHeaders);
  headers.set('Content-Type', 'application/octet-stream');
  headers.set('Content-Length', String(body.byteLength));
  headers.set('Connection', 'close');
  headers.set('Cache-Control', 'no-transform');
  headers.set('Access-Control-Allow-Origin', '*');
  return new Response(body, { status: 200, headers });
}

async function proxyBinary(downloadUrl, ctx) {
  if (!downloadUrl || !downloadUrl.startsWith(BINARY_URL_PREFIX)) {
    return new Response('Invalid download URL', { status: 400 });
  }

  const cacheKey = binaryCacheRequest(downloadUrl);
  const cache = caches.default;
  let cachedResponse = await cache.match(cacheKey);

  if (cachedResponse) {
    const cacheExpiry = cachedResponse.headers.get('X-Cache-Expiry');
    if (cacheExpiry && Date.now() < parseInt(cacheExpiry, 10)) {
      const buffer = await cachedResponse.arrayBuffer();
      return binaryResponse(buffer, {
        'X-Cache-Hit': 'true',
        'X-Cache-Time': cachedResponse.headers.get('X-Cache-Time'),
        'X-Cache-Expiry': cacheExpiry,
      });
    }
  }

  const response = await fetch(downloadUrl);
  const now = Date.now();
  const cacheExpiry = now + CACHE_TTL_MS;
  const rateLimitReset = response.headers.get('x-ratelimit-reset');

  if (response.status === 403 || response.status === 429) {
    if (cachedResponse) {
      const buffer = await cachedResponse.arrayBuffer();
      return binaryResponse(buffer, {
        'X-Cache-Hit': 'true',
        'X-Cache-Time': cachedResponse.headers.get('X-Cache-Time'),
        'X-Rate-Limited': 'true',
        'X-Rate-Limit-Reset': rateLimitReset || 'unknown',
      });
    }
    return new Response(
      JSON.stringify({
        error: 'Rate limit exceeded and no cached firmware available',
        retryAfter: rateLimitReset,
      }),
      {
        status: 429,
        headers: {
          'Content-Type': 'application/json',
          'Retry-After': response.headers.get('retry-after') || '1800',
        },
      }
    );
  }

  if (!response.ok) {
    const buffer = await response.arrayBuffer();
    return new Response(buffer, {
      status: response.status,
      headers: {
        'Content-Type': response.headers.get('Content-Type') || 'application/octet-stream',
        'Access-Control-Allow-Origin': '*',
      },
    });
  }

  const buffer = await response.arrayBuffer();
  const responseToCache = new Response(buffer, {
    status: 200,
    headers: {
      'Content-Type': 'application/octet-stream',
      'Content-Length': String(buffer.byteLength),
      'X-Cache-Expiry': cacheExpiry.toString(),
      'X-Cache-Time': new Date(now).toISOString(),
      'Cache-Control': 'public, max-age=1800',
    },
  });
  ctx.waitUntil(cache.put(cacheKey, responseToCache));

  return binaryResponse(buffer, {
    'X-Cache-Hit': 'false',
    'X-Cache-Time': new Date(now).toISOString(),
  });
}

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const firmware = url.searchParams.get('firmware');
    let tag = url.searchParams.get('tag');
    // PlatformIO may pass RELEASE_TAG_NAME with surrounding quotes in the URL.
    if (tag && tag.startsWith('"') && tag.endsWith('"')) {
      tag = tag.slice(1, -1);
    }
    // Blank tag (test/release builds with empty sysenv.release_tag_name) = outdated.
    if (tag == null) {
      tag = '';
    }

    if (!firmware) {
      return new Response(
        JSON.stringify({ error: 'Missing required query param: firmware' }),
        { status: 400, headers: { 'Content-Type': 'application/json' } }
      );
    }

    if (!FIRMWARE_NAME_RE.test(firmware)) {
      return new Response(JSON.stringify({ error: 'Invalid firmware name' }), {
        status: 400,
        headers: { 'Content-Type': 'application/json' },
      });
    }

    const releaseResult = await fetchCachedReleasesJson();
    if (!releaseResult.ok) {
      const headers = { 'Content-Type': 'application/json' };
      if (releaseResult.status === 429) {
        headers['Retry-After'] = '1800';
      }
      return new Response(releaseResult.body, {
        status: releaseResult.status,
        headers,
      });
    }

    const match = findReleaseWithFirmware(releaseResult.releases, firmware);
    if (!match) {
      return new Response(
        JSON.stringify({ error: 'firmware asset not found', firmware }),
        { status: 404, headers: { 'Content-Type': 'application/json' } }
      );
    }

    // Up to date only relative to the newest release that actually ships this firmware.
    const tagName = match.release.tag_name;
    if (tagName === tag) {
      return new Response(null, {
        status: 204,
        headers: {
          'X-Latest-Tag': tagName,
          'Access-Control-Allow-Origin': '*',
        },
      });
    }

    return proxyBinary(match.asset.browser_download_url, ctx);
  },
};
