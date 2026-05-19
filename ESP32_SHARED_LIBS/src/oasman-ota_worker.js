/**
 * OASMan unified OTA worker
 * GET ?firmware=<FIRMWARE_RELEASE_NAME>&tag=<RELEASE_TAG_NAME>
 * - 204 if latest release tag matches tag (already up to date)
 * - 200 + firmware binary if a newer release is available
 * Deploy as: oasman-ota → http://oasman-ota.gopro2027.workers.dev/
 */

const RELEASES_LATEST_URL =
  'https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest';

const BINARY_URL_PREFIX =
  'https://github.com/gopro2027/ArduinoAirSuspensionController/releases/download/';

const FIRMWARE_NAME_RE = /^[a-zA-Z0-9_]+$/;
const CACHE_TTL_MS = 30 * 60 * 1000;
/** Bump when binary cache/response format changes (v1 streamed bodies broke ESP32 HTTPClient). */
const BINARY_CACHE_VERSION = 'v2-buffered';

async function fetchCachedReleaseJson() {
  const cacheKey = new Request(RELEASES_LATEST_URL, { method: 'GET' });
  const cache = caches.default;
  let cachedResponse = await cache.match(cacheKey);

  if (cachedResponse) {
    const cacheExpiry = cachedResponse.headers.get('X-Cache-Expiry');
    if (cacheExpiry && Date.now() < parseInt(cacheExpiry, 10)) {
      const buffer = await cachedResponse.arrayBuffer();
      return {
        ok: true,
        release: JSON.parse(new TextDecoder().decode(buffer)),
        fromCache: true,
      };
    }
  }

  const response = await fetch(RELEASES_LATEST_URL, {
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
    if (cachedResponse) {
      const cachedBuffer = await cachedResponse.arrayBuffer();
      return {
        ok: true,
        release: JSON.parse(new TextDecoder().decode(cachedBuffer)),
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

  const responseToCache = new Response(buffer, {
    status: response.status,
    headers: {
      'Content-Type': 'application/json',
      'X-Cache-Expiry': cacheExpiry.toString(),
      'X-Cache-Time': new Date(now).toISOString(),
      'Cache-Control': 'public, max-age=1800',
    },
  });
  await cache.put(cacheKey, responseToCache.clone());

  return {
    ok: true,
    release: JSON.parse(new TextDecoder().decode(buffer)),
    fromCache: false,
  };
}

function findFirmwareAsset(release, firmware) {
  const assetName = `${firmware}_firmware.bin`;
  const assets = release.assets || [];
  for (const asset of assets) {
    if (asset.name === assetName) {
      return asset;
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

  const cacheKey = new Request(`${downloadUrl}#${BINARY_CACHE_VERSION}`, { method: 'GET' });
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

    if (!firmware || !tag) {
      return new Response(
        JSON.stringify({ error: 'Missing required query params: firmware, tag' }),
        { status: 400, headers: { 'Content-Type': 'application/json' } }
      );
    }

    if (!FIRMWARE_NAME_RE.test(firmware)) {
      return new Response(JSON.stringify({ error: 'Invalid firmware name' }), {
        status: 400,
        headers: { 'Content-Type': 'application/json' },
      });
    }

    const releaseResult = await fetchCachedReleaseJson();
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

    const release = releaseResult.release;
    const tagName = release.tag_name;

    if (tagName === tag) {
      return new Response(null, {
        status: 204,
        headers: {
          'X-Latest-Tag': tagName,
          'Access-Control-Allow-Origin': '*',
        },
      });
    }

    const asset = findFirmwareAsset(release, firmware);
    if (!asset || !asset.browser_download_url) {
      return new Response(
        JSON.stringify({ error: 'firmware asset not found', firmware, tag: tagName }),
        { status: 404, headers: { 'Content-Type': 'application/json' } }
      );
    }

    return proxyBinary(asset.browser_download_url, ctx);
  },
};
