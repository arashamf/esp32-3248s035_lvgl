#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
//-------------------------------------------------------------
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
//-------------------------------------------------------------
esp_err_t   wifi_setting_init       (void)      ;
esp_err_t   wifi_connect            (void)      ;
void        create_wifi_task        (void)      ;
void        wifi_task   (void *pvParameters)    ;
bool wifi_manager_is_connected      (void)      ;
bool wifi_manager_is_connecting     (void)      ;
bool wifi_manager_has_failed        (void)      ;

//-------------------------------------------------------------
//extern EventGroupHandle_t s_wifi_event_group;

#endif /* MAIN_WIFI_H_ */
