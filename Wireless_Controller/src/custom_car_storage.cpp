/**
 * DISCLAIMER: This file was fully AI generated
 * This file is used to store the custom car and wheel images in SPIFFS.
 */

#include "custom_car_storage.h"

#ifndef SCREEN_MODE_CIRCLE

#include <preferencable.h>
#include <esp_heap_caps.h>

static const char *kCarPath = "/custom/car.bin";
static const char *kWheelsPath = "/custom/wheels.bin";
static const char *kCarTmpPath = "/custom/car.tmp";
static const char *kWheelsTmpPath = "/custom/wheels.tmp";

struct CustomCarFileHeader
{
    char magic[4]; // "OASC"
    uint8_t version;
    uint8_t reserved[3];
    uint16_t w;
    uint16_t h;
    uint32_t data_size;
};

static_assert(sizeof(CustomCarFileHeader) == 16, "CustomCarFileHeader must be 16 bytes");

static lv_image_dsc_t g_runtimeCar = {};
static lv_image_dsc_t g_runtimeWheels = {};
static uint8_t *g_runtimeCarData = nullptr;
static uint8_t *g_runtimeWheelsData = nullptr;
static bool g_runtimeLoaded = false;
static bool g_fsMounted = false;

LV_IMG_DECLARE(img_car);
LV_IMG_DECLARE(img_wheels);

static bool validatePayload(uint16_t w, uint16_t h, size_t len)
{
    if (w == 0 || h == 0)
        return false;
    const size_t expected = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    return len == expected;
}

static uint8_t *allocImageBuffer(size_t len)
{
    uint8_t *buf = static_cast<uint8_t *>(heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf)
        buf = static_cast<uint8_t *>(heap_caps_malloc(len, MALLOC_CAP_8BIT));
    return buf;
}

static void freeRuntimeBuffers()
{
    if (g_runtimeCarData)
    {
        heap_caps_free(g_runtimeCarData);
        g_runtimeCarData = nullptr;
    }
    if (g_runtimeWheelsData)
    {
        heap_caps_free(g_runtimeWheelsData);
        g_runtimeWheelsData = nullptr;
    }
    memset(&g_runtimeCar, 0, sizeof(g_runtimeCar));
    memset(&g_runtimeWheels, 0, sizeof(g_runtimeWheels));
    g_runtimeLoaded = false;
}

static bool loadOneImage(const char *path, lv_image_dsc_t &dsc, uint8_t *&dataOut)
{
    File probe = SPIFFS.open(path, "r");
    if (!probe)
        return false;
    const size_t fileSize = probe.size();
    probe.close();

    if (fileSize < sizeof(CustomCarFileHeader))
        return false;

    uint8_t *fileBuf = allocImageBuffer(fileSize);
    if (!fileBuf)
        return false;

    const size_t got = readBytes(path, fileBuf, fileSize);
    if (got != fileSize)
    {
        heap_caps_free(fileBuf);
        return false;
    }

    CustomCarFileHeader hdr;
    memcpy(&hdr, fileBuf, sizeof(hdr));
    if (memcmp(hdr.magic, "OASC", 4) != 0 || hdr.version != 1 || !validatePayload(hdr.w, hdr.h, hdr.data_size))
    {
        heap_caps_free(fileBuf);
        return false;
    }
    if (fileSize != sizeof(hdr) + hdr.data_size)
    {
        heap_caps_free(fileBuf);
        return false;
    }

    uint8_t *data = allocImageBuffer(hdr.data_size);
    if (!data)
    {
        heap_caps_free(fileBuf);
        return false;
    }
    memcpy(data, fileBuf + sizeof(hdr), hdr.data_size);
    heap_caps_free(fileBuf);

    dataOut = data;
    dsc.header.cf = LV_COLOR_FORMAT_RGB565A8;
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.w = hdr.w;
    dsc.header.h = hdr.h;
    dsc.data_size = hdr.data_size;
    dsc.data = data;
    return true;
}

static bool saveImageFile(const char *finalPath, const char *tmpPath, const uint8_t *data, size_t len, uint16_t w, uint16_t h)
{
    if (!validatePayload(w, h, len))
        return false;

    CustomCarFileHeader hdr = {};
    memcpy(hdr.magic, "OASC", 4);
    hdr.version = 1;
    hdr.w = w;
    hdr.h = h;
    hdr.data_size = static_cast<uint32_t>(len);

    const size_t total = sizeof(hdr) + len;
    uint8_t *fileBuf = allocImageBuffer(total);
    if (!fileBuf)
        return false;

    memcpy(fileBuf, &hdr, sizeof(hdr));
    memcpy(fileBuf + sizeof(hdr), data, len);

    deleteFile(tmpPath);
    writeBytes(tmpPath, fileBuf, total, "w");
    heap_caps_free(fileBuf);

    if (!SPIFFS.exists(tmpPath))
        return false;

    deleteFile(finalPath);
    if (!SPIFFS.rename(tmpPath, finalPath))
    {
        deleteFile(tmpPath);
        return false;
    }
    return true;
}

bool customCarStorageInit()
{
    g_fsMounted = SPIFFS.begin(true);
    if (!g_fsMounted)
    {
        log_e("SPIFFS mount failed");
        return false;
    }

    freeRuntimeBuffers();

    if (!SPIFFS.exists(kCarPath) || !SPIFFS.exists(kWheelsPath))
        return true;

    if (!loadOneImage(kCarPath, g_runtimeCar, g_runtimeCarData))
    {
        freeRuntimeBuffers();
        return true;
    }
    if (!loadOneImage(kWheelsPath, g_runtimeWheels, g_runtimeWheelsData))
    {
        freeRuntimeBuffers();
        return true;
    }

    g_runtimeLoaded = true;
    log_i("Custom car images loaded from SPIFFS (%ux%u car, %ux%u wheels)",
          g_runtimeCar.header.w, g_runtimeCar.header.h,
          g_runtimeWheels.header.w, g_runtimeWheels.header.h);
    return true;
}

bool customCarHasImages()
{
    return g_runtimeLoaded;
}

const lv_image_dsc_t *getPresetCarImage()
{
    if (g_runtimeLoaded)
        return &g_runtimeCar;
    return &img_car;
}

const lv_image_dsc_t *getPresetWheelsImage()
{
    if (g_runtimeLoaded)
        return &g_runtimeWheels;
    return &img_wheels;
}

bool customCarSaveCar(const uint8_t *data, size_t len, uint16_t w, uint16_t h)
{
    if (!g_fsMounted)
        return false;
    return saveImageFile(kCarPath, kCarTmpPath, data, len, w, h);
}

bool customCarSaveWheels(const uint8_t *data, size_t len, uint16_t w, uint16_t h)
{
    if (!g_fsMounted)
        return false;
    return saveImageFile(kWheelsPath, kWheelsTmpPath, data, len, w, h);
}

void customCarClear()
{
    if (g_fsMounted)
    {
        deleteFile(kCarPath);
        deleteFile(kWheelsPath);
        deleteFile(kCarTmpPath);
        deleteFile(kWheelsTmpPath);
    }
    freeRuntimeBuffers();
}

#endif /* !SCREEN_MODE_CIRCLE */
