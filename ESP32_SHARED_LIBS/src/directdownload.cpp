#include "directdownload.h"

// PlatformIO release envs pass -D RELEASE_TAG_NAME=${sysenv.release_tag_name}.
// When that env var is unset, the macro is defined but empty — treat like missing.
#define RELEASE_TAG_NAME_EMPTY_HELPER(x) x##1
#define RELEASE_TAG_NAME_IS_EMPTY(x) RELEASE_TAG_NAME_EMPTY_HELPER(x)
#if !defined(RELEASE_TAG_NAME) || (RELEASE_TAG_NAME_IS_EMPTY(RELEASE_TAG_NAME) == 1)
#undef RELEASE_TAG_NAME
#define RELEASE_TAG_NAME build-dev
#endif

#define download_firmware_response_success 1
#define download_firmware_response_retry 2
#define download_firmware_response_fail -1

/* WiFi.status() is not enough to tell "still trying" from "we were rejected":
 * the core's STA.cpp maps 4WAY_HANDSHAKE_TIMEOUT to WL_DISCONNECTED, the same
 * value it reports mid-connect. The disconnect reason is the only place that
 * distinction survives, so we capture it from the event. */
enum WIFI_FAIL_CLASS
{
    WIFI_FAIL_NONE,
    WIFI_FAIL_TRANSIENT,  // worth retrying: weak signal, busy AP, dropped frame
    WIFI_FAIL_PASSWORD,   // wrong passphrase, retrying can never fix it
    WIFI_FAIL_NO_NETWORK  // SSID not present, or no compatible security
};

static volatile WIFI_FAIL_CLASS wifiFailReason = WIFI_FAIL_NONE;

// event callback for disconnect event during wifi connection establishment
// only sets the value of wifiFailReason
static void onWifiStaDisconnected(arduino_event_id_t event, arduino_event_info_t info)
{
    uint8_t reason = info.wifi_sta_disconnected.reason;

    // Reason 8 is our own WiFi.disconnect() in connectToWifi, not a failure.
    if (reason == WIFI_REASON_ASSOC_LEAVE)
        return;

    switch (reason)
    {
    /* The passphrase is only ever exercised in the 4-way EAPOL handshake. A wrong
     * PSK derives a PMK whose MIC the AP rejects, and the AP then simply stops
     * replying -- so a bad password surfaces as a timeout rather than an explicit
     * rejection. Association itself succeeds either way, which is why AUTH_FAIL
     * is almost never what you actually see. */
    case WIFI_REASON_MIC_FAILURE:            // 14
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: // 15
    case WIFI_REASON_AUTH_FAIL:              // 202
    case WIFI_REASON_HANDSHAKE_TIMEOUT:      // 204
        wifiFailReason = WIFI_FAIL_PASSWORD;
        break;
    case WIFI_REASON_NO_AP_FOUND: // 201
    /* 210/211 exist only in IDF 5.x (the controller). This file also compiles for
     * the manifold on arduino-esp32 2.0.17 / IDF 4.4, where those enum names are
     * undefined, so match them numerically. */
    case 210: // NO_AP_FOUND_W_COMPATIBLE_SECURITY
    case 211: // NO_AP_FOUND_IN_AUTHMODE_THRESHOLD
        wifiFailReason = WIFI_FAIL_NO_NETWORK;
        break;
    default:
        // Includes 212 NO_AP_FOUND_IN_RSSI_THRESHOLD: the AP is there, just weak.
        wifiFailReason = WIFI_FAIL_TRANSIENT;
        break;
    }

    log_i("Wifi disconnect reason %u -> fail class %d", reason, (int)wifiFailReason);
}

int connectToWifi(String SSID, String PASS)
{
    static bool wifiEventRegistered = false;
    if (!wifiEventRegistered)
    {
        WiFi.onEvent(onWifiStaDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        wifiEventRegistered = true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // manually control reconnects
    WiFi.disconnect();
    delay(100); // let any event from the previous attempt drain before we listen
    log_i("Connecting to network");
    wifiFailReason = WIFI_FAIL_NONE;
    WiFi.begin(SSID, PASS);

    const int maxtimeout = 20; // 500ms * 20 = 10 seconds
    int timeoutCounter = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        // if wifiFailReason is not none, that means we have disconnected (ARDUINO_EVENT_WIFI_STA_DISCONNECTED event ran) so we should return, because setAutoReconnect is false so it's not going to automatically retry
        if (wifiFailReason != WIFI_FAIL_NONE)
        {
            return download_firmware_response_retry;
        }
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
    https->setReuse(false); // helps with stability on flaky wifi connections. This is automatically set to true if useHTTP10 is true, but added here for clarity.
    https->setTimeout(65535); // 65535 milliseconds = 65.535 seconds = 1 minute and 5.535 seconds
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
    int definitiveFailures = 0;

    while (connectToWifi(SSID, PASS) != download_firmware_response_success)
    {
        /* A wrong passphrase or an absent SSID will never start working on retry,
         * so give up instead of burning the whole ~63s budget on it. 2 failures is
         * allowed before hard reboot: a weak signal can drop a handshake frame and look exactly
         * like a bad password on a single attempt. */
        if (wifiFailReason == WIFI_FAIL_PASSWORD || wifiFailReason == WIFI_FAIL_NO_NETWORK)
        {
            definitiveFailures++;
            if (definitiveFailures > 1)
            {
                bool badPassword = (wifiFailReason == WIFI_FAIL_PASSWORD);
                log_i("Wifi rejected us (%s), not retrying.", badPassword ? "bad password" : "network not found");
                setupdateResult(badPassword
                                    ? UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_PASSWORD
                                    : UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_NO_NETWORK);
                ESP.restart();
                return;
            }
        }

        counter++;
        // transient failures 5 times allowed since it's much more likely not that we are manually controlling the wifi restarts
        if (counter > 4)
        {
            log_i("Failed to connect to wifi after multiple attempts.");
            setupdateResult(UPDATE_STATUS::UPDATE_STATUS_FAIL_WIFI_CONNECTION);
            ESP.restart();
            return;
        }

        log_i("Retrying to connect to wifi...");
        delay(2500);

        
    }

    /* Modem sleep during TLS often shows up as HTTPC_ERROR_CONNECTION_LOST (-5). */
    WiFi.setSleep(false);
    // Connected: give reconnects back to the core so a drop mid-download recovers.
    WiFi.setAutoReconnect(true);

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
