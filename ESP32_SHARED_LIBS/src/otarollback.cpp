#include "otarollback.h"

#include <esp_ota_ops.h>

// Must be extern "C" (the weak default is in the core's esp32-hal-misc.c, unmangled) and must
// stay in the same file as otaVerifyLoop(), or the linker never pulls it out of the archive.
// By default the bootloader will cancel the rollback via it's own checks inside of the bootloader code, but we want to wait to cancel the rollback in our own code to make sure setup runs before we cancel the rollback. So this function tells the bootloader 'hey! I will manually decide when to cancel the rollback and mark the update as valid.'
extern "C" bool verifyRollbackLater()
{
    return true;
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

    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        log_i("OTA verify: new image ran %lums, confirm result %d", OTA_VERIFY_CONFIRM_MS, (int)err);
    }
}
