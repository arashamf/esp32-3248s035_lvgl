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
//#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"

#define RED_LED_GPIO        CONFIG_RED_LED_GPIO
#define RED_BLUE_GPIO       CONFIG_BLUE_LED_GPIO
#define RED_GREEN_GPIO      CONFIG_GREEN_LED_GPIO
#define BLINK_PERIOD        CONFIG_BLINK_PERIOD

static const char *TAG = "main";
#ifdef  CONFIG_LED_ENABLE
const enum          {
    RED_LED     = 0 ,
    BLUE_LED        ,
    GREEN_LED       ,
    ALL_LED_PINS    ,
}   led_pins_t ;
const gpio_num_t pin_LED[ALL_LED_PINS] = { RED_LED_GPIO, RED_BLUE_GPIO, RED_GREEN_GPIO };

//------------------------------------------------------------------------------------------------//
void task_blink_led  (void *pvParameters) ;
static esp_err_t switch_led  (uint8_t led, uint32_t state);
static esp_err_t disable_all_led  (void);
static void configure_led(void);

//------------------------------------------------------------------------------------------------//
static esp_err_t switch_led  (uint8_t led, uint32_t state) {
    esp_err_t ret = ESP_OK;
    if (led >= ALL_LED_PINS)     { return ESP_ERR_INVALID_ARG; }
    ESP_RETURN_ON_ERROR ( gpio_set_level(pin_LED[led], (state&0x01)), TAG, "wrong argument for LED");
    return ret;
}

//------------------------------------------------------------------------------------------------//
static esp_err_t disable_all_led  (void) {
    esp_err_t ret = ESP_OK;
    for (uint8_t count = 0; count < ALL_LED_PINS; count++)  {
        ESP_RETURN_ON_ERROR ( gpio_set_level(pin_LED[count], 0), TAG, "wrong argument for LED");
    }
    return ret;
}

//------------------------------------------------------------------------------------------------//
static void configure_led(void) {
    for (uint8_t count = 0; count < ALL_LED_PINS; count++)  {
        gpio_reset_pin(pin_LED[count]);
        gpio_set_direction((pin_LED[count]), GPIO_MODE_OUTPUT);
        gpio_set_level((pin_LED[count]) ,    0);
    }
    xTaskCreate(task_blink_led, "blink_led", 1024, NULL, 6, NULL);
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
    ret=init_lcd();
    if (ret != ESP_OK) {    ESP_LOGE(TAG, "LCD initialization failed: 0x%04x", ret);    }
    #if CONFIG_TOUCH_ENABLE
    ESP_ERROR_CHECK(init_touch());
    #endif
    ret=init_lvgl();
    if (ret != ESP_OK) {    ESP_LOGE(TAG, "LVGL initialization failed: 0x%04x", ret);   }
    //run_display();
    run_demo_UI ();

    //настройка wifi и lwip
    // If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
    // and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. 
    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {  esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL); }
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    create_wifi_task();
    xTaskCreate(udp_task, "udp_task", 2*1024, NULL, 5, NULL);
    xTaskCreate(ntp_task, "ntp_task", 2*1024, NULL, 6, NULL);
    vTaskDelay(500/ portTICK_PERIOD_MS);
    #ifdef  CONFIG_LED_ENABLE
    configure_led(); // Configure the peripheral according to the LED type 
    #endif

    while (1)   {
        ESP_LOGI(TAG, "Heap free size:%d", xPortGetFreeHeapSize());
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

//------------------------------------------------------------------------------------------------//
#ifdef  CONFIG_LED_ENABLE
void task_blink_led (void *pvParameters) {
    while(1)    {
        for (uint8_t count = 0; count < ALL_LED_PINS; count++)  {
            disable_all_led  ();
            switch_led  (count, 1);
            vTaskDelay(BLINK_PERIOD/portTICK_PERIOD_MS);
        }
    }
}
#endif