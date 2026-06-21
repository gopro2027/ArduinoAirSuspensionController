/**
 * DISCLAIMER: This file was fully AI generated
 * We only expose one function which is used to receive the images from the OAS-Man Car Creator web tool over serial.
 */

#include "serial_image_upload.h"

#ifndef SCREEN_MODE_CIRCLE

#include "custom_car_storage.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <driver/usb_serial_jtag.h>

#pragma region generic serial functions

static bool s_ready = false;

bool uploadSerialInit()
{
    if (s_ready)
        return true;

    usb_serial_jtag_driver_config_t cfg = {};
    cfg.tx_buffer_size = 256;
    cfg.rx_buffer_size = 4096;
    const esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        log_e("uploadSerialInit: usb_serial_jtag_driver_install failed (%d)", (int)err);
        return false;
    }

    s_ready = true;
    return true;
}

void uploadSerialWrite(const char *data, size_t len)
{
    if (!s_ready || !data || len == 0)
        return;
    usb_serial_jtag_write_bytes(data, len, pdMS_TO_TICKS(1000));
}

void uploadSerialWriteLine(const char *line)
{
    if (!line)
        return;
    uploadSerialWrite(line, strlen(line));
    uploadSerialWrite("\n", 1);
}

int uploadSerialRead()
{
    if (!s_ready)
        return -1;

    uint8_t c = 0;
    const int n = usb_serial_jtag_read_bytes(&c, 1, 0);
    return n > 0 ? static_cast<int>(c) : -1;
}

bool uploadSerialReadExact(uint8_t *dst, size_t len, unsigned long deadlineMs)
{
    if (!s_ready || !dst)
        return false;

    size_t got = 0;
    while (got < len)
    {
        if (millis() > deadlineMs)
            return false;

        const int n = usb_serial_jtag_read_bytes(dst + got, len - got, pdMS_TO_TICKS(10));
        if (n > 0)
            got += static_cast<size_t>(n);
        else
            delay(1);
    }
    return true;
}

#pragma endregion

static const unsigned long kUploadTimeoutMs = 10UL * 60UL * 1000UL;

static void sendLine(const char *msg)
{
    uploadSerialWriteLine(msg);
}

static void sendErr(const char *msg)
{
    uploadSerialWrite("ERR ", 4);
    uploadSerialWriteLine(msg);
}

static uint8_t *allocUploadBuffer(size_t len)
{
    uint8_t *buf = static_cast<uint8_t *>(heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf)
        buf = static_cast<uint8_t *>(heap_caps_malloc(len, MALLOC_CAP_8BIT));
    return buf;
}

static bool handleCarBegin(uint16_t w, uint16_t h, size_t size, unsigned long deadlineMs)
{
    uint8_t *payload = allocUploadBuffer(size);
    if (!payload)
    {
        sendErr("car alloc failed");
        return false;
    }
    const bool okRead = uploadSerialReadExact(payload, size, deadlineMs);
    const bool okSave = okRead && customCarSaveCar(payload, size, w, h);
    heap_caps_free(payload);
    if (!okRead)
    {
        sendErr("car receive timeout");
        return false;
    }
    if (!okSave)
    {
        sendErr("car save failed");
        return false;
    }
    sendLine("CAR_OK");
    return true;
}

static bool handleWheelsBegin(uint16_t w, uint16_t h, size_t size, unsigned long deadlineMs)
{
    uint8_t *payload = allocUploadBuffer(size);
    if (!payload)
    {
        sendErr("wheels alloc failed");
        return false;
    }
    const bool okRead = uploadSerialReadExact(payload, size, deadlineMs);
    const bool okSave = okRead && customCarSaveWheels(payload, size, w, h);
    heap_caps_free(payload);
    if (!okRead)
    {
        sendErr("wheels receive timeout");
        return false;
    }
    if (!okSave)
    {
        sendErr("wheels save failed");
        return false;
    }
    sendLine("WHEELS_OK");
    return true;
}

bool serialImageUploadRun()
{
    if (!uploadSerialInit())
    {
        log_e("serialImageUploadRun: uploadSerialInit failed");
        return false;
    }

    const unsigned long deadlineMs = millis() + kUploadTimeoutMs;
    String lineBuf;

    sendLine("OASMAN_IMG_READY");

    while (millis() < deadlineMs)
    {
        const int c = uploadSerialRead();
        if (c < 0)
        {
            delay(1);
            continue;
        }

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            lineBuf.trim();
            if (lineBuf.length() == 0)
                continue;

            if (lineBuf.startsWith("CAR_BEGIN "))
            {
                uint16_t w = 0;
                uint16_t h = 0;
                unsigned long size = 0;
                if (sscanf(lineBuf.c_str(), "CAR_BEGIN %hu %hu %lu", &w, &h, &size) != 3 || size == 0)
                {
                    sendErr("bad CAR_BEGIN");
                    lineBuf = "";
                    continue;
                }
                lineBuf = "";
                if (!handleCarBegin(w, h, static_cast<size_t>(size), deadlineMs))
                    return false;
                continue;
            }

            if (lineBuf.startsWith("WHEELS_BEGIN "))
            {
                uint16_t w = 0;
                uint16_t h = 0;
                unsigned long size = 0;
                if (sscanf(lineBuf.c_str(), "WHEELS_BEGIN %hu %hu %lu", &w, &h, &size) != 3 || size == 0)
                {
                    sendErr("bad WHEELS_BEGIN");
                    lineBuf = "";
                    continue;
                }
                lineBuf = "";
                if (!handleWheelsBegin(w, h, static_cast<size_t>(size), deadlineMs))
                    return false;
                continue;
            }

            if (lineBuf == "DONE")
            {
                sendLine("DONE");
                return true;
            }

            if (lineBuf.startsWith("PING"))
            {
                sendLine("PONG");
                lineBuf = "";
                continue;
            }

            lineBuf = "";
            continue;
        }

        if (lineBuf.length() < 120)
            lineBuf += static_cast<char>(c);
    }

    sendErr("upload timeout");
    return false;
}

#endif /* !SCREEN_MODE_CIRCLE */
