
#include "wifi_tile.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lv_port.h"
#include "bsp_wifi.h"

static lv_obj_t *list;
lv_obj_t *lable_wifi_ip;

#define LIST_BTN_LEN_MAX 20
lv_obj_t *list_btns[LIST_BTN_LEN_MAX];
uint16_t list_item_count = 0;

bool g_wifi_enable = true;

SemaphoreHandle_t wifi_scanf_semaphore;

static void btn_wifi_scan_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED && g_wifi_enable)
    {
        for (int i = 0; i < list_item_count; i++)
        {
            lv_obj_del(list_btns[i]);
        }
        list_item_count = 0;
        list_btns[0] = lv_list_add_btn(list, NULL, "WiFi scanning underway!");

        xSemaphoreGive(wifi_scanf_semaphore);
        // app_wifi_scan((void*)wifi_infos, &list_item_count, 20);
        // for (int i = 0; i < list_item_count && i < LIST_BTN_LEN_MAX; i++)
        // {
        //     list_btns[i] = lv_list_add_btn(list, NULL, wifi_infos[i].name);
        //     label = lv_label_create(list_btns[i]);
        //     lv_label_set_text_fmt(label, "%d db", wifi_infos[i].rssi);
        //     list_item_count++;
        //     // lv_list_get_btn_index();
        // }
    }
}

static void sw_wifi_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        if (lv_obj_has_state(obj, LV_STATE_CHECKED))
        {
            g_wifi_enable = true;
            esp_wifi_connect();
        }
        else
        {
            g_wifi_enable = false;
            for (int i = 0; i < list_item_count; i++)
            {
                lv_obj_del(list_btns[i]);
            }
            list_item_count = 0;
            esp_wifi_disconnect();
        }
    }
}

static void lvgl_wifi_task(void *arg)
{
    char str[50] = {0};
    char str_wifi_ip[32] = {0};
    lv_obj_t *label;
    wifi_ap_record_t ap_info[LIST_BTN_LEN_MAX];

    while (1)
    {
        if (xSemaphoreTake(wifi_scanf_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            printf("wifi_scanf!!\r\n");
            memset(ap_info, 0, sizeof(ap_info));
            if (bsp_wifi_scan(ap_info, &list_item_count, LIST_BTN_LEN_MAX))
            {
                lv_obj_del(list_btns[0]);
                if (lvgl_port_lock(0))
                {
                    for (int i = 0; i < list_item_count && i < LIST_BTN_LEN_MAX; i++)
                    {
                        list_btns[i] = lv_list_add_btn(list, NULL, (char *)ap_info[i].ssid);
                        label = lv_label_create(list_btns[i]);
                        lv_label_set_text_fmt(label, "%d db", ap_info[i].rssi);
                    }
                    lvgl_port_unlock();
                }
            }
        }

        bsp_wifi_get_ip(str_wifi_ip);
        sprintf(str, "IP: %s", str_wifi_ip);

        if (lvgl_port_lock(0))
        {
            lv_label_set_text(lable_wifi_ip, str);
            lvgl_port_unlock();
        }
    }
}

void wifi_tile_init(lv_obj_t *parent)
{
    /*Create a list*/
    list = lv_list_create(parent);

    wifi_scanf_semaphore = xSemaphoreCreateBinary();

    lv_obj_t *lable = lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "WiFi");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lable_wifi_ip = lv_label_create(parent);
    lv_label_set_text(lable_wifi_ip, "IP: 0.0.0.0");
    lv_obj_align(lable_wifi_ip, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *btn = lv_btn_create(parent);
    lable = lv_label_create(btn);
    lv_label_set_text(lable, "Scan");
    lv_obj_center(lable);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 20, 5);

    lv_obj_add_event_cb(btn, btn_wifi_scan_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_add_event_cb(sw, sw_wifi_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_state(sw, LV_STATE_CHECKED);

    lv_obj_set_size(list, lv_pct(95), lv_pct(85));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);

    xTaskCreatePinnedToCore(lvgl_wifi_task, "lvgl_wifi_task", 1024 * 5, NULL, 0, NULL, 1);
}