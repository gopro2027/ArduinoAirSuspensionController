/**
 * GitHub Release List HTTP Proxy Worker
 * First it checks if we have a valid cached response and returns it if so.
 * If not, it fetches the latest release info from GitHub API. Then:
 * If rate limit is exceeded, it falls back to any cached response if available. If no cache is available, it returns a rate limit error.
 * If the github request is successful, it caches the response for 30 minutes and returns it.
 */

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    
    // Extract the GitHub URL from query parameter
    // const targetUrl = url.searchParams.get('url');
    const targetUrl = "https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest";
    
    if (!targetUrl || !targetUrl.startsWith('https://api.github.com/')) {
      return new Response('Invalid URL', { status: 400 });
    }
    
    // Create a cache key based on the target URL
    const cacheKey = new Request(targetUrl, { method: 'GET' });
    const cache = caches.default;
    
    // Check if we have a valid cached response
    let cachedResponse = await cache.match(cacheKey);
    
    if (cachedResponse) {
      // Check if cache is still valid based on our custom header
      const cacheExpiry = cachedResponse.headers.get('X-Cache-Expiry');
      const cacheTime = cachedResponse.headers.get('X-Cache-Time');
      
      if (cacheExpiry && Date.now() < parseInt(cacheExpiry)) {
        console.log('Returning cached response (still valid for ' + 
          Math.round((parseInt(cacheExpiry) - Date.now()) / 1000 / 60) + ' more minutes)');
        
        // Return cached data with original content
        const buffer = await cachedResponse.arrayBuffer();
        return new Response(buffer, {
          status: cachedResponse.status,
          headers: {
            'Content-Type': 'application/json',
            'X-Cache-Hit': 'true',
            'X-Cache-Time': cacheTime,
            'X-Cache-Expiry': cacheExpiry
          }
        });
      }
    }
    
    // Fetch from GitHub with HTTPS
    const response = await fetch(targetUrl, {
      headers: {
        'User-Agent': 'ESP32-Firmware-Updater',
        'Accept': 'application/vnd.github+json'
      }
    });
    
    // Read the response
    const buffer = await response.arrayBuffer();
    const now = Date.now();
    const thirtyMinutes = 30 * 60 * 1000;
    const cacheExpiry = now + thirtyMinutes;
    
    // Check rate limit headers
    const rateLimitRemaining = response.headers.get('x-ratelimit-remaining');
    const rateLimitReset = response.headers.get('x-ratelimit-reset');
    
    console.log(`Rate limit remaining: ${rateLimitRemaining}`);
    console.log(`Rate limit resets at: ${rateLimitReset ? new Date(parseInt(rateLimitReset) * 1000).toISOString() : 'unknown'}`);
    
    // Handle rate limit exceeded (status 403 or 429)
    if (response.status === 403 || response.status === 429) {
      console.log('Rate limit exceeded, checking for cached response');
      
      // If we have any cached response (even expired), return it
      if (cachedResponse) {
        const cachedBuffer = await cachedResponse.arrayBuffer();
        const cachedTime = cachedResponse.headers.get('X-Cache-Time');
        return new Response(cachedBuffer, {
          status: 200, // Return 200 since we're providing valid cached data
          headers: {
            'Content-Type': 'application/json',
            'X-Cache-Hit': 'true',
            'X-Cache-Time': cachedTime,
            'X-Rate-Limited': 'true',
            'X-Rate-Limit-Reset': rateLimitReset || 'unknown'
          }
        });
      }
      
      // No cache available, return rate limit error
      return new Response(JSON.stringify({
        error: 'Rate limit exceeded and no cached data available',
        retryAfter: rateLimitReset
      }), {
        status: 429,
        headers: {
          'Content-Type': 'application/json',
          'Retry-After': response.headers.get('retry-after') || '1800' // 30 minutes in seconds
        }
      });
    }
    
    // If successful, cache the response for 30 minutes
    if (response.ok) {
      // Create response to cache with 30-minute expiry
      const responseToCache = new Response(buffer, {
        status: response.status,
        headers: {
          'Content-Type': 'application/json',
          'X-Cache-Expiry': cacheExpiry.toString(),
          'X-Cache-Time': new Date(now).toISOString(),
          'Cache-Control': 'public, max-age=1800' // 30 minutes
        }
      });
      
      // Store in cache
      await cache.put(cacheKey, responseToCache.clone());
      console.log(`Cached response for 30 minutes until ${new Date(cacheExpiry).toISOString()}`);
    }
    
    // Return the response
    return new Response(buffer, {
      status: response.status,
      headers: {
        'Content-Type': 'application/json',
        'X-Cache-Hit': 'false',
        'X-Cache-Time': new Date(now).toISOString(),
        'X-Rate-Limit-Remaining': rateLimitRemaining || 'unknown',
        'X-Rate-Limit-Reset': rateLimitReset || 'unknown'
      }
    });
  }
};