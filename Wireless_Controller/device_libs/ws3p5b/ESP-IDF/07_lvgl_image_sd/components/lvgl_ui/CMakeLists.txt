
# 递归查找所有子目录下的.c文件
file(GLOB_RECURSE SRC_FILES "*.cpp")


idf_component_register(SRCS ${SRC_FILES}
                    INCLUDE_DIRS "."
                    REQUIRES "lvgl" "XPowersLib" "freertos" "spi_flash" "esp_psram" "driver" "esp_hw_support" "esp32-camera" "esp_lv_port" "esp_bsp")
