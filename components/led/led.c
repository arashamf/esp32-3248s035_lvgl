#include "freertos/FreeRTOS.h" //должен быть первым в списке, потомуиспользует vTaskDelay()
#include "freertos/task.h"

#include "led.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"

//------------------------------------------------------------------------------------------------//
#define RED_LED_GPIO        CONFIG_RED_LED_GPIO
#define BLUE_LED_GPIO       CONFIG_BLUE_LED_GPIO
#define GREEN_LED_GPIO      CONFIG_GREEN_LED_GPIO
#define BLINK_PERIOD        CONFIG_BLINK_PERIOD

//------------------------------------------------------------------------------------------------//
static const char *TAG = "LED";

#ifdef  CONFIG_LED_ENABLE
const enum          {
    RED_LED     = 0 ,
    BLUE_LED        ,
    GREEN_LED       ,
    ALL_LED_PINS    ,
}   led_pins_t      ;
const gpio_num_t pin_LED[ALL_LED_PINS] = { RED_LED_GPIO, BLUE_LED_GPIO, GREEN_LED_GPIO };

//------------------------------------------------------------------------------------------------//
static  esp_err_t   switch_led      (uint8_t led, uint32_t state)  ;
static  esp_err_t   disable_all_led (void)                         ;
        void        task_blink_led  (void *pvParameters)           ;
//------------------------------------------------------------------------------------------------//
static esp_err_t switch_led  (uint8_t led, uint32_t state) {
    esp_err_t ret = ESP_OK;
    if (led >= ALL_LED_PINS)     { return ESP_ERR_INVALID_ARG; }
    ESP_RETURN_ON_ERROR ( gpio_set_level(pin_LED[led], (state&0x01)), TAG, "wrong argument for LED");
    return ret;
}

//------------------------------------------------------------------------------------------------//
static esp_err_t disable_all_led  (void)    {
    esp_err_t ret = ESP_OK;
    for (uint8_t count = 0; count < ALL_LED_PINS; count++)  {
        ESP_RETURN_ON_ERROR ( gpio_set_level(pin_LED[count], 0), TAG, "wrong argument for LED");
    }
    return ret;
}

//------------------------------------------------------------------------------------------------//
void configure_led(void) {
    for (uint8_t count = 0; count < ALL_LED_PINS; count++)      {
        gpio_reset_pin(pin_LED[count])                          ;
        gpio_set_direction((pin_LED[count]), GPIO_MODE_OUTPUT)  ;
        gpio_set_level((pin_LED[count]) ,    0)                 ;
    }
    xTaskCreate(task_blink_led, "blink_led", 1024, NULL, 6, NULL);
}

//------------------------------------------------------------------------------------------------//
void task_blink_led (void *pvParameters)                    {
    while(1)    {
        for (uint8_t count = 0; count < ALL_LED_PINS; count++)  {
            disable_all_led  ();
            switch_led  (count, 1);
            vTaskDelay(BLINK_PERIOD/portTICK_PERIOD_MS);
        }
    }
}

#endif