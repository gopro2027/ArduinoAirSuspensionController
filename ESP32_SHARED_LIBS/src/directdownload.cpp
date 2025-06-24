#include "directdownload.h"

void downloadUpdate(String SSID, String PASS)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("Connecting to network");
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
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION);
            ESP.restart();
            return;
        }
    }

    Serial.println("Downloading version file");
    WiFiClientSecure client;
    client.setInsecure();

    char versionNum[10];
    HTTPClient https;
    if (https.begin(client, "https://oasman.dev/oasman/version.txt"))
    {
        int httpResponseCode = https.GET();
        if (httpResponseCode == HTTP_CODE_OK)
        {

            strcpy(versionNum, https.getString().c_str());
            Serial.println(httpResponseCode);
            Serial.println(versionNum);
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
            ESP.restart();
            return;
        }
        https.end();
    }
    else
    {
        Serial.println("Connection failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_VERSION_REQUEST);
        ESP.restart();
        return;
    }

    char url[100];
    snprintf(url, sizeof(url), DOWNLOAD_FIRMWARE_BIN_URL, versionNum);
    Serial.println(url);
    if (https.begin(client, url))
    {
        int httpCode = https.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            int fileSize = https.getSize(); // Total size in bytes
            Serial.printf("File size: %d bytes\n", fileSize);

            if (fileSize < 1000)
            {
                Serial.println("Error with file size. Aborting update");
                setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
                ESP.restart();
                return;
            }

            WiFiClient *stream = https.getStreamPtr();

            const size_t bufferSize = 1024; // 1KB chunks
            uint8_t buffer[bufferSize];

            size_t totalRead = 0;
            unsigned long lastTime = millis();

            if (Update.begin(fileSize))
            { // UPDATE_SIZE_UNKNOWN could also be used

                while ((https.connected()) && totalRead < fileSize)
                { //  stream->available()
                    size_t bytesRead = stream->readBytes(buffer, bufferSize);

                    // Something has gone wrong....
                    if (bytesRead == 0)
                    {
                        break;
                    }

                    // Do something with the data chunk
                    Serial.printf("Read %u bytes\n", bytesRead);

                    // For example: write to SPIFFS, SD, or process in RAM
                    totalRead += bytesRead;

                    if (Update.write(buffer, bytesRead) != bytesRead)
                    {
                        Update.printError(Serial);
                    }

                    // Optional: avoid watchdog reset
                    if (millis() - lastTime > 1000)
                    {
                        delay(10);
                        lastTime = millis();
                    }
                }
            }

            Serial.printf("Total bytes read: %u\n", totalRead);
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
        Serial.println("Connection failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_FILE_REQUEST);
        ESP.restart();
        return;
    }

    if (Update.end())
    {
        // Update successful
        Serial.println("Update was successful!");
    }
    else
    {
        // Update failed
        Serial.println("Update download failed");
        setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_GENERIC);
        ESP.restart();
        return;
    }

    setupdateResult(UPDATE_STATUS::UPDATE_STATUS_SUCCESS);

    ESP.restart();
}