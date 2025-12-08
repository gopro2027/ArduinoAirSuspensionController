export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    
    // Extract the GitHub URL from query parameter
    const targetUrl = url.searchParams.get('url');
    
    if (!targetUrl || !targetUrl.startsWith('https://github.com/gopro2027/ArduinoAirSuspensionController/releases/download/')) {
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
        console.log('Returning cached file (still valid for ' + 
          Math.round((parseInt(cacheExpiry) - Date.now()) / 1000 / 60) + ' more minutes)');
        
        // Return cached file
        return new Response(cachedResponse.body, {
          status: cachedResponse.status,
          headers: {
            'Content-Type': cachedResponse.headers.get('Content-Type') || 'application/octet-stream',
            'Content-Length': cachedResponse.headers.get('Content-Length'),
            'Access-Control-Allow-Origin': '*',
            'X-Cache-Hit': 'true',
            'X-Cache-Time': cacheTime,
            'X-Cache-Expiry': cacheExpiry
          }
        });
      }
    }
    
    // Fetch from GitHub with HTTPS
    const response = await fetch(targetUrl);
    
    const now = Date.now();
    const thirtyMinutes = 30 * 60 * 1000;
    const cacheExpiry = now + thirtyMinutes;
    
    // Check rate limit headers (GitHub does apply rate limits to raw file downloads)
    const rateLimitRemaining = response.headers.get('x-ratelimit-remaining');
    const rateLimitReset = response.headers.get('x-ratelimit-reset');
    
    if (rateLimitRemaining) {
      console.log(`Rate limit remaining: ${rateLimitRemaining}`);
      console.log(`Rate limit resets at: ${rateLimitReset ? new Date(parseInt(rateLimitReset) * 1000).toISOString() : 'unknown'}`);
    }
    
    // Handle rate limit exceeded (status 403 or 429)
    if (response.status === 403 || response.status === 429) {
      console.log('Rate limit exceeded, checking for cached file');
      
      // If we have any cached response (even expired), return it
      if (cachedResponse) {
        const cachedTime = cachedResponse.headers.get('X-Cache-Time');
        return new Response(cachedResponse.body, {
          status: 200, // Return 200 since we're providing valid cached data
          headers: {
            'Content-Type': cachedResponse.headers.get('Content-Type') || 'application/octet-stream',
            'Content-Length': cachedResponse.headers.get('Content-Length'),
            'Access-Control-Allow-Origin': '*',
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
      // Clone the response to cache it (since we can only read the body once)
      const responseToCache = new Response(response.body, {
        status: response.status,
        headers: {
          'Content-Type': response.headers.get('Content-Type') || 'application/octet-stream',
          'Content-Length': response.headers.get('Content-Length'),
          'X-Cache-Expiry': cacheExpiry.toString(),
          'X-Cache-Time': new Date(now).toISOString(),
          'Cache-Control': 'public, max-age=1800' // 30 minutes
        }
      });
      
      // Store in cache - we need to clone because we're returning one copy and caching another
      const [responseToReturn, responseToCacheClone] = [responseToCache.clone(), responseToCache.clone()];
      
      // Cache in background
      ctx.waitUntil(cache.put(cacheKey, responseToCacheClone));
      console.log(`Cached file for 30 minutes until ${new Date(cacheExpiry).toISOString()}`);
      
      // Return the response
      return new Response(responseToReturn.body, {
        status: responseToReturn.status,
        headers: {
          'Content-Type': response.headers.get('Content-Type') || 'application/octet-stream',
          'Content-Length': response.headers.get('Content-Length'),
          'Access-Control-Allow-Origin': '*',
          'X-Cache-Hit': 'false',
          'X-Cache-Time': new Date(now).toISOString(),
          'X-Rate-Limit-Remaining': rateLimitRemaining || 'unknown',
          'X-Rate-Limit-Reset': rateLimitReset || 'unknown'
        }
      });
    }
    
    // Return error response if not successful and not rate limited
    return new Response(response.body, {
      status: response.status,
      headers: {
        'Content-Type': response.headers.get('Content-Type') || 'application/octet-stream',
        'Access-Control-Allow-Origin': '*',
        'X-Cache-Hit': 'false'
      }
    });
  }
};