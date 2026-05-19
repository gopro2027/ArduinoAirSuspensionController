#include "directdownload.h"

#ifndef RELEASE_TAG_NAME
#define RELEASE_TAG_NAME build-dev
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

HTTPClient *https;

bool check300Redirect(int httpCode, String &responseURLString)
{
    if (httpCode / 100 == 3) /* any 300 code lets try to redirect */
    {
        responseURLString = https->getLocation();
        log_i("Redirected to: %s", responseURLString.c_str());
        https->end();
        return true;
    }
    return false;
}

int installFirmware(String &url)
{
    if (!https->begin(url))
    {
        log_i("Connection failed");
        return download_firmware_response_retry;
    }

    https->useHTTP10(true); // not necessary, but tells the server we don't support chunked responses
    https->setReuse(false); // helps with stability on flaky wifi connections
    https->setTimeout(300000); // 5 minute timeout for firmware download
    const char *headerKeys[] = {"Content-Length"};
    https->collectHeaders(headerKeys, 1);// may help ensure content length is stored by the HTTPClient
    int httpCode = https->GET();
    log_i("HTTP GET code: %d", httpCode);

    if (check300Redirect(httpCode, url)) /* any 300 code lets try to redirect */
    {
        return download_firmware_response_retry;
    }

    if (httpCode == HTTP_CODE_NO_CONTENT)
    {
        log_i("Already up to date (204). Installed tag matches latest release.");
        https->end();
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_ALREADY_UP_TO_DATE);
        ESP.restart();
        return download_firmware_response_fail;
    }

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP GET failed, error: %d\n", httpCode);
        https->end();
        return download_firmware_response_retry;
    }

    int fileSize = https->getSize(); // Total size in bytes
    if (fileSize <= 0)
    {
        const String cl = https->header("Content-Length");
        if (cl.length() > 0)
            fileSize = cl.toInt();
    }
    log_i("File size: %d bytes\n", fileSize);

    if (fileSize < 1000)
    {
        log_i("Missing Content-Length (chunked response?). Aborting update");
        https->end();
        return download_firmware_response_retry;
    }

    log_i("=== Before Update.begin ===");
    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    log_i("Min free heap: %d", ESP.getMinFreeHeap());

    if (!Update.begin(fileSize))
    {
        log_i("Update.begin failed");
        https->end();
        return download_firmware_response_retry;
    }

    size_t written = Update.writeStream(*https->getStreamPtr());

    if (written != (size_t)fileSize)
    {
        log_i("Written only : %d/%d. Retry?", written, fileSize);
        https->end();
        Update.abort();
        return download_firmware_response_retry;
    }
    log_i("Written : %d successfully", written);

    https->end();

    if (Update.end())
    {
        log_i("Update was successful!");
    }
    else
    {
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
    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    log_i("Min free heap: %d", ESP.getMinFreeHeap());

    btStop();
    log_i("Bluetooth stopped");

    log_i("=== After bluetooth stop Memory Status ===");
    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Largest free block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    log_i("Min free heap: %d", ESP.getMinFreeHeap());

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

    /* Modem sleep during TLS often shows up as HTTPC_ERROR_CONNECTION_LOST (-5). */
    WiFi.setSleep(false);

    https = new HTTPClient();

    String url = String("http://oasman-ota.gopro2027.workers.dev/?firmware=") +
                 String(FIRMWARE_RELEASE_NAME) +
                 "&tag=" + String(EVALUATE_AND_STRINGIFY(RELEASE_TAG_NAME));

    log_i("Downloading firmware from %s", url.c_str());

    counter = 0;
    while (installFirmware(url) != download_firmware_response_success)
    {
        if (counter > 5)
        {
            log_i("Failed to download firmware after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
            ESP.restart();
            return;
        }
        log_i("Firmware install requested retry...");
        delay(5000);

        counter++;
    }

    ESP.restart();
    return;
}

/**
 * Cloudflare Worker: oasman-ota
 * http://oasman-ota.gopro2027.workers.dev/?firmware=<FIRMWARE_RELEASE_NAME>&tag=<RELEASE_TAG_NAME>
 * See file: oasman-ota_worker.js
 *
 * Returns 204 when tag matches latest release (already up to date).
 * Returns 200 + firmware binary when an update is available.
 */
