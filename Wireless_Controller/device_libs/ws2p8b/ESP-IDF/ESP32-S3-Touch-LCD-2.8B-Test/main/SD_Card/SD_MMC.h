
#pragma once

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "esp_flash.h"

#include "EXIO/TCA9554PWR.h"        

#define CONFIG_EXAMPLE_PIN_CLK  2
#define CONFIG_EXAMPLE_PIN_CMD  1
#define CONFIG_EXAMPLE_PIN_D0   42
#define CONFIG_EXAMPLE_PIN_D1   -1
#define CONFIG_EXAMPLE_PIN_D2   -1
#define CONFIG_EXAMPLE_PIN_D3   -1  // Using EXIO


esp_err_t SD_Card_CS_EN(void);
esp_err_t SD_Card_CS_Dis(void);

esp_err_t s_example_write_file(const char *path, char *data);
esp_err_t s_example_read_file(const char *path);

extern uint32_t SDCard_Size;
extern uint32_t Flash_Size;
void SD_Init(void);
void Flash_Searching(void);