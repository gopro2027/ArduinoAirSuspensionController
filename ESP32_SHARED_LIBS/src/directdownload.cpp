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

String getDownloadFirmwareURL(WiFiClientSecure &client, HTTPClient &https)
{

    log_i("Downloading json from github api releases");

    if (https.begin(client, "https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases/latest"))
    {
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
            DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

            if (error)
            {
                log_i("JSON parse error: %s", error.c_str());
                https.end();
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
                ESP.restart();
            }

            for (int j = 0; j < doc["assets"].size(); j++)
            {
                log_i("Checking asset %d: %s", j, doc["assets"][j]["name"].as<const char *>());

                if (doc["assets"][j]["name"] == String(FIRMWARE_RELEASE_NAME) + String("_") + String("firmware.bin"))
                {
                    const char *url = doc["assets"][j]["browser_download_url"];
                    log_i("Found firmware download url: %s", url);
                    https.end();
                    doc.clear();
                    return String(url);
                }
            }

            log_i("Could not find correct firmware download url in github api response");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            ESP.restart();
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
}

void installFirmware(WiFiClientSecure &client, HTTPClient &https, String url)
{
    if (https.begin(client, url))
    {
        int httpCode = https.GET();
        log_i("HTTP GET code: %d", httpCode);
        if (httpCode == HTTP_CODE_FOUND)
        {
            String redirectUrl = https.getLocation();
            log_i("Redirected to: %s", redirectUrl.c_str());
            https.end();
            installFirmware(client, https, redirectUrl); // recursive ass shit hopefully it works fine I don't feel like changing it
            ESP.restart();
            return;
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
                return;
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
                    return;
                }
            }
            else
            {
                log_i("File too large");
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
                ESP.restart();
                return;
            }
        }
        else
        {
            Serial.printf("HTTPS GET failed, error: %s\n", https.errorToString(httpCode).c_str());
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
            ESP.restart();
            return;
        }
        https.end();
    }
    else
    {
        log_i("Connection failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
        ESP.restart();
        return;
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
        return;
    }

    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_SUCCESS);

    ESP.restart();
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
    HTTPClient https;
    // https.setReuse(false); // Don't reuse connections
    // https.setTimeout(15000);

    String url = getDownloadFirmwareURL(client, https);
    log_i("Downloading firmware from %s", url.c_str());

    installFirmware(client, https, url);

    ESP.restart();
    return;
}