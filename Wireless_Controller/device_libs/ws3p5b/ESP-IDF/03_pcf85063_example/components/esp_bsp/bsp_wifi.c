#include "bsp_wifi.h"


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

#define DEFAULT_SCAN_LIST_SIZE 10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

SemaphoreHandle_t wifi_scan_Semaphore = NULL;
SemaphoreHandle_t wifi_connect_Semaphore = NULL;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            // esp_wifi_connect();
            // s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG, "WiFi scan done!!!!!");
        // 释放信号量
        xSemaphoreGive(wifi_scan_Semaphore);
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        // 释放信号量
        xSemaphoreGive(wifi_connect_Semaphore);
        wifi_ap_record_t ap_info;
        esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);

        if (err == ESP_OK)
        {
            // Print the SSID
            ESP_LOGI(TAG, "Connected SSID: %s", ap_info.ssid);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get AP info");
        }
    }
}

/* Initialize Wi-Fi as sta and set scan method */
bool bsp_wifi_scan(wifi_ap_record_t *ap_info, uint16_t *scan_number, uint16_t scan_max_num)
{
    // uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    // wifi_ap_record_t ap_info[scan_max_num];
    esp_err_t ret = ESP_OK;
    // memset(ap_info, 0, sizeof(ap_info));

    ret = esp_wifi_scan_start(NULL, false);
    if (ESP_OK != ret)
        goto scan_err;

    // ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);

    if (xSemaphoreTake(wifi_scan_Semaphore, pdMS_TO_TICKS(5000)))
    {
        esp_wifi_scan_stop();
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(scan_number));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&scan_max_num, ap_info));
        ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", *scan_number, scan_max_num);
        for (int i = 0; i < scan_max_num && i < *scan_number; i++)
        {
            ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
            // memcpy(infos[i].name, ap_info[i].ssid, strlen((char *)ap_info[i].ssid));
            ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
            // infos[i].rssi = ap_info[i].rssi;
        }
        if (*scan_number > scan_max_num)
        {
            *scan_number = scan_max_num;
        }
        return true;
    }
scan_err:
    *scan_number = 0;
    return false;
}

// esp_err_t bsp_wifi_disconnect(void)
// {
//     return esp_wifi_disconnect();
// }

// esp_err_t bsp_wifi_connect(void)
// {
//     return esp_wifi_connect();
// }

esp_err_t bsp_wifi_sta_connect(const char *ssid, const char *password)
{
    esp_wifi_disconnect();
    // esp_wifi_stop();
    wifi_config_t wifi_config = {};
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_wifi_connect();

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;

    // if (xSemaphoreTake(wifi_connect_Semaphore, pdMS_TO_TICKS(5000)))
    //     return ESP_OK;
    // else
    //     return ESP_FAIL;
}


void bsp_wifi_get_ip(char *ip)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    sprintf(ip, "%d.%d.%d.%d", IP2STR(&ip_info.ip));
}

void bsp_wifi_init(const char *ssid, const char *pass)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();
    wifi_scan_Semaphore = xSemaphoreCreateBinary();
    wifi_connect_Semaphore = xSemaphoreCreateBinary();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    if (ssid != NULL && pass != NULL)
    {
        bsp_wifi_sta_connect(ssid, pass);
    }
}