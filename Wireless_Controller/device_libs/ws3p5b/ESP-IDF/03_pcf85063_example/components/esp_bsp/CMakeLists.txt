file(GLOB_RECURSE SRC_FILES  "*.c" "*.cpp")

idf_component_register(SRCS ${SRC_FILES}
                    INCLUDE_DIRS "."
                    REQUIRES "esp_lcd" "driver" "esp_adc" "espressif__esp_lcd_axs15231b"  "fatfs" "nvs_flash" "lwip" "esp_wifi" "XPowersLib" "esp32-camera" "espressif__esp_codec_dev")
