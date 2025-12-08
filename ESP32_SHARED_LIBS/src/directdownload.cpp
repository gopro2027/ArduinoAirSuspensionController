#include "directdownload.h"

#ifndef RELEASE_TAG_NAME
#define RELEASE_TAG_NAME "build-dev" 
#endif

#define download_firmware_response_success 1
#define download_firmware_response_retry 2
#define download_firmware_response_fail -1

int connectToWifi(String SSID, String PASS)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    log_i("Connecting to network");
    WiFi.begin(SSID, PASS);

    const int maxtimeout = 20; // 500ms * 20 = 10 seconds
    int timeoutCounter = 0;

    // WiFi.setTxPower(WIFI_POWER_19_5dBm);

    // WiFi.setSleep(false);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        timeoutCounter++;
        if (timeoutCounter > maxtimeout)
        {
            return download_firmware_response_retry;
        }
    }

    return download_firmware_response_success;
}

HTTPClient https;

bool check300Redirect(int httpCode, String &responseURLString)
{
    if (httpCode / 100 == 3) /* any 300 code lets try to redirect */
    {
        responseURLString = https.getLocation();
        log_i("Redirected to: %s", responseURLString.c_str());
        https.end();
        return true;
    }
    return false;
}

int getDownloadFirmwareURL(String &responseURLString)
{
    static bool rateLimited = false;
    log_i("Downloading json from github api releases");

    if (rateLimited) {
        log_i("Using direct secure connection to github api due to prior rate limiting");

        WiFiClientSecure client;
        client.setInsecure();
        
        if (!https.begin(client, "https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest"))
        {
            log_i("Connection failed");
            return download_firmware_response_retry;
        }
    } else {
        if (!https.begin("http://githubreleaselist-http-proxy.gopro2027.workers.dev/"))
        {
            log_i("Connection failed");
            return download_firmware_response_retry;
        }
    }

    delay(50);
    int httpResponseCode = https.GET();
    if (check300Redirect(httpResponseCode, responseURLString)) /* any 300 code lets try to redirect */
    {
        return download_firmware_response_retry;
    }

    if (httpResponseCode == HTTP_CODE_FORBIDDEN || httpResponseCode == HTTP_CODE_TOO_MANY_REQUESTS)
    {
        rateLimited = true;
        log_i("Rate limited by GitHub API, switching to direct secure connection for future requests");
        log_i("HTTP Response code: %d", httpResponseCode);
        log_i("Error Response: %s", https.getString().c_str());
        https.end();
        return download_firmware_response_retry;
    }

    if (httpResponseCode != HTTP_CODE_OK)
    {
        log_i("HTTP Response code: %d", httpResponseCode);
        log_i("Error Response: %s", https.getString().c_str());
        https.end();
        return download_firmware_response_retry;
    }

    JsonDocument doc;
    log_i("Got 200 response about to deserialize json");
    log_i("looking for %s in the json", FIRMWARE_RELEASE_NAME);

    JsonDocument filter;
    filter["tag_name"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["browser_download_url"] = true;

    // Stream parse instead of loading entire string
    WiFiClient *stream = https.getStreamPtr();
    delay(50);

    // Wait for data to be available (with timeout)
    unsigned long timeout = millis();
    while (stream->available() == 0)
    {
        if (millis() - timeout > 5000)
        { // 5 second timeout
            log_i("Timeout waiting for steam data");
            https.end();
            return download_firmware_response_retry;
        }
        delay(10);
    }

    delay(10);
    DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

    if (error)
    {
        log_i("JSON parse error: %s", error.c_str());
        https.end();
        return download_firmware_response_retry;
    }

    delay(10);
    if (doc["tag_name"] == String(EVALUATE_AND_STRINGIFY(RELEASE_TAG_NAME)))
    {
        log_i("Latest already installed. Found tag_name name %s matches installed %s.", doc["tag_name"].as<const char *>(), EVALUATE_AND_STRINGIFY(RELEASE_TAG_NAME));
        https.end();
        doc.clear();
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_ALREADY_UP_TO_DATE);
        ESP.restart();
        return download_firmware_response_fail;
    }

    log_i("Found update: %s. Current update: %s", doc["tag_name"].as<const char *>(), EVALUATE_AND_STRINGIFY(RELEASE_TAG_NAME));

    for (int j = 0; j < doc["assets"].size(); j++)
    {
        log_i("Checking asset %d: %s", j, doc["assets"][j]["name"].as<const char *>());
        delay(10);

        if (doc["assets"][j]["name"] == String(FIRMWARE_RELEASE_NAME) + String("_") + String("firmware.bin"))
        {
            const char *url = doc["assets"][j]["browser_download_url"];
            log_i("Found firmware download url: %s", url);
            https.end();
            doc.clear();
            responseURLString = String(url);
            return download_firmware_response_success;
        }
    }

    log_i("Could not find correct firmware download url in github api response, retrying...");
    https.end();
    doc.clear();

    return download_firmware_response_retry;
}

int installFirmware(String &url)
{
    if (!https.begin(url))
    {
        log_i("Connection failed");
        return download_firmware_response_retry;
    }

    int httpCode = https.GET();
    log_i("HTTP GET code: %d", httpCode);

    if (check300Redirect(httpCode, url)) /* any 300 code lets try to redirect */
    {
        return download_firmware_response_retry;
    }

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTPS GET failed, error: %d\n", httpCode);
        https.end();
        return download_firmware_response_retry;
    }

    int fileSize = https.getSize(); // Total size in bytes
    log_i("File size: %d bytes\n", fileSize);

    if (fileSize < 1000)
    {
        log_i("Error with file size. Aborting update");
        https.end();
        return download_firmware_response_retry;
    }

    log_i("=== Before Update.begin ===");
    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    log_i("Min free heap: %d", ESP.getMinFreeHeap());

    if (!Update.begin(fileSize))
    {
        log_i("Update.begin failed");
        https.end();
        return download_firmware_response_retry;
    }

    size_t written = Update.writeStream(https.getStream());

    if (written == fileSize)
    {
        log_i("Written : %d successfully", written);
    }
    else
    {
        log_i("Written only : %d/%d. Retry?", written, fileSize);
        https.end();
        Update.abort();
        return download_firmware_response_retry;
    }

    https.end();

    if (Update.end())
    {
        // Update successful
        log_i("Update was successful!");
    }
    else
    {
        // Update failed
        log_i("Update download failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_GENERIC);
        ESP.restart();
        return download_firmware_response_fail;
    }

    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_SUCCESS);
    delay(500);

    ESP.restart();

    return download_firmware_response_success;
}

void downloadUpdate(String SSID, String PASS)
{

    log_i("=== Initial Memory Status ===");
    log_i("Free heap: %d", ESP.getFreeHeap());                                          // 119468
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)); // 106484
    log_i("Min free heap: %d", ESP.getMinFreeHeap());                                   // 119260

    btStop();
    log_i("Bluetooth stopped");

    // Print detailed heap information
    log_i("=== After bluetooth stop Memory Status ===");
    log_i("Free heap: %d", ESP.getFreeHeap());                                          // 134940
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)); // 106484
    log_i("Min free heap: %d", ESP.getMinFreeHeap());                                   // 119260
    // heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    int counter = 0;

    while (connectToWifi(SSID, PASS) != download_firmware_response_success)
    {
        if (counter > 3)
        {
            log_i("Failed to connect to wifi after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION);
            ESP.restart();
            return;
        }

        log_i("Retrying to connect to wifi...");
        delay(2500);

        counter++;
    }

    String url;
    int code = 0;
    counter = 0;
    while (getDownloadFirmwareURL(url) != download_firmware_response_success)
    {
        if (counter > 5)
        {
            log_i("Failed to get firmware download URL after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            ESP.restart();
            return;
        }

        log_i("Retrying to get firmware download URL...");
        delay(5000);

        counter++;
    }

    log_i("Downloading firmware from %s", url.c_str());

    url = String("http://githubreleasebinary-http-proxy.gopro2027.workers.dev/?url=") + url;

    counter = 0;
    while (installFirmware(url) != download_firmware_response_success)
    {
        if (counter > 5)
        {
            log_i("Failed to download firmware after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
            ESP.restart();
        }
        log_i("Firmware install requested retry...");
        delay(5000);

        counter++;
    }

    ESP.restart();
    return;
}

/**
 * 
 * Cloudflare Worker code for the proxy:
 * http://githubreleasebinary-http-proxy.gopro2027.workers.dev/?url=
 *  See file: githubreleasebinary-http-proxy_worker.js
 * 
 *  http proxy for the list of releases:
 *  http://githubreleaselist-http-proxy.gopro2027.workers.dev/?url=https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest
 *  See file: githubreleaselist-http-proxy_worker.js
 *
 *  Memory difference notes:
 *  Before (using https, aka WiFiClientSecure on the .begin function): 
 *  Free heap: 32980
 *  Largest free block: 17396
 *  Min free heap: 12780
 *
 *  After (cloudflare proxy using http):
 *  Free heap: 78256
 *  Largest free block: 34804
 *  Min free heap: 13884
 * 
 *  After adding second proxy for the releases list:
 *  Free heap: 80348
 *  Largest free block: 65524
 *  Min free heap: 76444
 *
 *
 */