#include "otarollback.h"

#include <esp_ota_ops.h>

// Must be extern "C" (the weak default is in the core's esp32-hal-misc.c, unmangled) and must
// stay in the same file as otaVerifyLoop(), or the linker never pulls it out of the archive.
extern "C" bool verifyRollbackLater()
{
    return true;
}

bool otaVerifyIsPending()
{
    esp_ota_img_states_t state;
    return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
           state == ESP_OTA_IMG_PENDING_VERIFY;
}

void otaVerifyConfirmNow()
{
    if (otaVerifyIsPending())
    {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        log_i("OTA verify: new image confirmed, result %d", (int)err);
    }
}

void otaVerifyLoop()
{
    static const unsigned long firstLoopMs = millis();
    static bool done = false;
    if (done || millis() - firstLoopMs < OTA_VERIFY_CONFIRM_MS)
    {
        return;
    }
    done = true;
    otaVerifyConfirmNow();
}
