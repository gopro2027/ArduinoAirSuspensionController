#include "directdownload.h"
#include <ArduinoJson.h>

bool connectToWifi(String SSID, String PASS)
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
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION);
            ESP.restart();
            return false;
        }
    }

    return true;
}

#define download_firmware_response_success 1
#define download_firmware_response_retry 2
#define download_firmware_response_fail -1
int getDownloadFirmwareURL(WiFiClientSecure &client, String &responseURLString)
{
    HTTPClient https;
    log_i("Downloading json from github api releases");

    if (https.begin(client, "https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest"))
    {
        delay(50);
        int httpResponseCode = https.GET();
        if (httpResponseCode == HTTP_CODE_OK)
        {

            JsonDocument doc;
            log_i("Got 200 response about to deserialize json");
            log_i("looking for %s in the json", FIRMWARE_RELEASE_NAME);

            JsonDocument filter;
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
                    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
                    ESP.restart();
                }
                delay(10);
            }

            delay(10);
            DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

            if (error)
            {
                log_i("JSON parse error: %s", error.c_str());
                https.end();
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
                ESP.restart();
            }

            delay(10);
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
            // delay(500);
            // return getDownloadFirmwareURL(client, https, count + 1); // ugh I hate recursion
            return download_firmware_response_retry;

            // log_i("Could not find correct firmware download url in github api response");
            // setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            // ESP.restart();
        }
        else
        {
            Serial.print("Error code: ");
            log_i("%d", httpResponseCode);
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            ESP.restart();
        }
    }
    else
    {
        log_i("Connection failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
        ESP.restart();
    }
    return download_firmware_response_fail;
}

int installFirmware(WiFiClientSecure &client, String &url)
{
    HTTPClient https;
    if (https.begin(client, url))
    {
        int httpCode = https.GET();
        log_i("HTTP GET code: %d", httpCode);
        if (httpCode == HTTP_CODE_FOUND)
        {
            String redirectUrl = https.getLocation();
            log_i("Redirected to: %s", redirectUrl.c_str());
            https.end();
            url = redirectUrl;
            return download_firmware_response_retry;
        }

        if (httpCode == HTTP_CODE_OK)
        {
            int fileSize = https.getSize(); // Total size in bytes
            log_i("File size: %d bytes\n", fileSize);

            if (fileSize < 1000)
            {
                log_i("Error with file size. Aborting update");
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
                ESP.restart();
            }

            if (Update.begin(fileSize))
            { // UPDATE_SIZE_UNKNOWN could also be used
                size_t written = Update.writeStream(https.getStream());

                if (written == fileSize)
                {
                    log_i("Written : %d successfully", written);
                }
                else
                {
                    log_i("Written only : %d/%d. Retry?", written, fileSize);
                    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
                    ESP.restart();
                }
            }
            else
            {
                log_i("File too large");
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
                ESP.restart();
            }
        }
        else
        {
            Serial.printf("HTTPS GET failed, error: %s\n", https.errorToString(httpCode).c_str());
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
            ESP.restart();
        }
        https.end();
    }
    else
    {
        log_i("Connection failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
        ESP.restart();
    }

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
    }

    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_SUCCESS);

    ESP.restart();

    return download_firmware_response_success;
}

void downloadUpdate(String SSID, String PASS)
{

    if (!connectToWifi(SSID, PASS))
    {
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    // client.setTimeout(120000); // 120 second timeout
    // HTTPClient https;
    // https.setReuse(false); // Don't reuse connections
    // https.setTimeout(15000);

    String url;
    int code = 0;
    int counter = 0;
    while (code != download_firmware_response_success)
    {
        counter++;
        code = getDownloadFirmwareURL(client, url);
        if (code == download_firmware_response_retry)
        {
            log_i("Retrying to get firmware download URL...");
            delay(1000);
        }
        else if (counter > 5 || code == download_firmware_response_fail)
        {
            log_i("Failed to get firmware download URL after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            ESP.restart();
            return;
        }
    }

    log_i("Downloading firmware from %s", url.c_str());

    // ESP.restart();
    // return;

    while (installFirmware(client, url) == download_firmware_response_retry)
    {
        log_i("302 received, retrying firmware download from new URL: %s", url.c_str());
        delay(1000);
    }

    ESP.restart();
    return;
}