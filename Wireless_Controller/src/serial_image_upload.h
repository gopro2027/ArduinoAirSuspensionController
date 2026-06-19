#ifndef serial_image_upload_h
#define serial_image_upload_h

#include <stddef.h>
#include <stdint.h>

#ifndef SCREEN_MODE_CIRCLE

/** Block until upload completes, fails, or times out. Returns true on success. */
bool serialImageUploadRun();

#endif /* !SCREEN_MODE_CIRCLE */

#endif /* serial_image_upload_h */
