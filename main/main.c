/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "udp.h"
#include "ntp.h"
#include "wifi.h"
#include <string.h>
#include "display.h"
#include "esp_log.h"
#include "lwip/sys.h"
#include "lwip/err.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "esp_lvgl_port.h"

#define BLINK_GPIO      CONFIG_BLINK_GPIO
#define BLINK_PERIOD    CONFIG_BLINK_PERIOD

static const char *TAG = "main";

#ifdef CONFIG_BLINK_GPIO
//------------------------------------------------------------------------------------------------//
void task_blink_led  (void *pvParameters) ;

//------------------------------------------------------------------------------------------------//
static void blink_led(uint32_t state) { gpio_set_level(BLINK_GPIO, (state&0x01)); }    //Set the GPIO level according to the state (LOW or HIGH)

//------------------------------------------------------------------------------------------------//
static void configure_led(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);   //Set the GPIO as a push/pull output 
    gpio_set_level(BLINK_GPIO, 0);
}
#endif

//------------------------------------------------------------------------------------------------//
void run_demo_UI (void) {
    while (!is_lvgl_ready()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    // Создаем простой интерфейс для проверки
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "ESP32-3248S035\nLVGL + WiFi");
    lv_obj_center(label);
    
    // Добавим кнопку для примера
    lv_obj_t *btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Press me!");
    lv_obj_center(btn_label);
}

//------------------------------------------------------------------------------------------------//
void app_main(void) {
    wifi_ap_record_t info;
    esp_err_t ret = nvs_flash_init();     //Initialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
        ret = nvs_flash_init();
        ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
    }
    ESP_ERROR_CHECK(ret);

    //запуск графики
    ESP_LOGI(TAG, "Initializing display...");   
    /*ret=init_lcd();
    if (ret != ESP_OK) {    ESP_LOGE(TAG, "LCD initialization failed: 0x%04x", ret);    }
    #if CONFIG_TOUCH_ENABLE
    ESP_ERROR_CHECK(init_touch());
    #endif
    ret=init_lvgl();
    if (ret != ESP_OK) {    ESP_LOGE(TAG, "LVGL initialization failed: 0x%04x", ret);   }*/
    run_display();
    run_demo_UI ();

    //настройка wifi и lwip
    // If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
    // and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. 
    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {  esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL); }
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta();
    xTaskCreate(udp_task, "udp_task", 2*1024, NULL, 4, NULL);
    xTaskCreate(ntp_task, "ntp_task", 2*1024, NULL, 5, NULL);

    configure_led(); // Configure the peripheral according to the LED type 
    uint8_t s_led_state = 1;
    blink_led(s_led_state&0x01); 
   // xTaskCreate(task_blink_led, "blink_led", 512, NULL, 6, NULL);

    while (1)   {
       ESP_LOGI(TAG, "Heap free size:%d", xPortGetFreeHeapSize());
       ret = esp_wifi_sta_get_ap_info(&info); //информация о точке доступа, с которой связано устройство
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "error! wifi_sta_get_ap_info: 0x%04x", ret); //вывод статуса соединения с точкой доступа
            wifi_init_sta();    //если нет соединения с с точкой доступа, попытка нового соединения
        }
        blink_led(s_led_state&0x01); 
        s_led_state = !s_led_state;  
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

//------------------------------------------------------------------------------------------------//
void task_blink_led (void *pvParameters) {
    uint8_t s_led_state = 0;
  //  configure_led(); // Configure the peripheral according to the LED type 
    while(1)    {
        blink_led(s_led_state);                  //Toggle the LED state
        s_led_state = !s_led_state;
        vTaskDelay(BLINK_PERIOD/portTICK_PERIOD_MS);
    }
}