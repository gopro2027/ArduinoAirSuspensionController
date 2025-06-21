#include "ota.h"

// Taken from here https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/OTAWebUpdater/OTAWebUpdater.ino

#define SSID_FORMAT "OASMAN-%06lX" // 12 chars total

WebServer server(80);
Ticker tkSecond;
uint8_t otaDone = 0;

const char *alphanum = "0123456789!@#$%^&*abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
String generatePass(uint8_t str_len)
{
    String buff;
    for (int i = 0; i < str_len; i++)
    {
        buff += alphanum[random(strlen(alphanum) - 1)];
    }
    return buff;
}

void apMode()
{
    char ssid[13];
    long unsigned int espmac = ESP.getEfuseMac() >> 24;
    snprintf(ssid, 13, SSID_FORMAT, espmac);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid); // Set up the SoftAP
    MDNS.begin("oasman");
}

void handleUpdateEnd()
{
    server.sendHeader("Connection", "close");
    if (Update.hasError())
    {
        server.send(502, "text/plain", Update.errorString());
    }
    else
    {
        server.sendHeader("Refresh", "10");
        server.sendHeader("Location", "/");
        server.send(307);
        ESP.restart();
    }
}

void handleUpdate()
{
    size_t fsize = UPDATE_SIZE_UNKNOWN;
    if (server.hasArg("size"))
    {
        fsize = server.arg("size").toInt();
    }
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.printf("Receiving Update: %s, Size: %d\n", upload.filename.c_str(), fsize);
        if (!Update.begin(fsize))
        {
            otaDone = 0;
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Update.printError(Serial);
        }
        else
        {
            otaDone = 100 * Update.progress() / Update.size();
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        {
            Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
        }
        else
        {
            Serial.printf("%s\n", Update.errorString());
            otaDone = 0;
        }
    }
}

void webServerInit()
{
    server.on(
        "/update", HTTP_POST,
        []()
        {
            handleUpdateEnd();
        },
        []()
        {
            handleUpdate();
        });
    server.on("/favicon.ico", HTTP_GET, []()
              {
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len); });
    server.onNotFound([]()
                      { server.send(200, "text/html", indexHtml); });
    server.begin();
    Serial.printf("Web Server ready at http://oasman.local or http://%s\n", WiFi.softAPIP().toString().c_str());
}

void everySecond()
{
    if (otaDone > 1)
    {
        Serial.printf("ota: %d%%\n", otaDone);
    }
}

void ota_setup()
{
    apMode();
    webServerInit();
    tkSecond.attach(1, everySecond);
}

void ota_loop()
{
    server.handleClient();
}