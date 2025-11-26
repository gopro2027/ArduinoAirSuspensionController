#ifndef __BSP_WIFI_H__
#define __BSP_WIFI_H__

#include <stdio.h>
#include "esp_wifi.h"


#ifdef __cplusplus
extern "C" {
#endif

void bsp_wifi_init(const char *ssid, const char *pass);
void bsp_wifi_get_ip(char *ip);
esp_err_t bsp_wifi_sta_connect(const char *ssid, const char *password);
bool bsp_wifi_scan(wifi_ap_record_t *ap_info, uint16_t *scan_number, uint16_t scan_max_num);

#ifdef __cplusplus
}
#endif

#endif