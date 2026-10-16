/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "udp.h"
#include "gui.h"
#include "led.h"
#include "ntp.h"
#include "wifi.h"
#include "display.h"
#include "esp_log.h"
#include "lwip/sys.h"
#include "lwip/err.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_system.h"
#include "esp_lvgl_port.h"

//------------------------------------------------------------------------------------------------//
static const char *TAG = "main";

//------------------------------------------------------------------------------------------------//
void app_main(void) {
    esp_err_t ret = nvs_flash_init();     //Initialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)   {
        ret = nvs_flash_erase();
        ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
        ret = nvs_flash_init();
        ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
    }
    ESP_ERROR_CHECK(ret);

    run_display();  
    gui_init(); //запуск графики
    //настройка wifi и lwip
    // If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
    // and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. 
    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {  esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL); }
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    create_wifi_task();
    xTaskCreate(udp_task, "udp_task", 2*1024, NULL, 5, NULL);
    xTaskCreate(ntp_task, "ntp_task", 4*1024, NULL, 6, NULL);

    #ifdef  CONFIG_LED_ENABLE
    configure_led(); // Configure the peripheral according to the LED type 
    #endif

    while (1)   {
        ESP_LOGI(TAG, "Heap free size:%d", xPortGetFreeHeapSize());
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

//------------------------------------------------------------------------------------------------//

