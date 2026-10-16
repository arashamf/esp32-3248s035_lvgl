#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gui.h"
#include "display.h"
#include "esp_log.h"

//------------------------------------------------------------------------------------------------//
static const char *TAG = "GUI";
//------------------------------------------------------------------------------------------------//
static void run_demo_UI (void) ;

//------------------------------------------------------------------------------------------------//
static  void run_demo_UI (void)             {
    while (!is_lvgl_ready()) {  vTaskDelay(pdMS_TO_TICKS(10));  }
    // Создание простого интерфейса для проверки
    lv_obj_t *label = lv_label_create(lv_screen_active())       ;
    lv_label_set_text(label, "ESP32-3248S035\nLVGL + WiFi")     ;
    lv_obj_center(label)                                        ;
    // Добавим кнопку для примера
    lv_obj_t *btn = lv_btn_create(lv_screen_active())           ;
    lv_obj_set_size(btn, 120, 50)                               ;
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30)              ;
    
    lv_obj_t *btn_label = lv_label_create(btn)                  ;
    lv_label_set_text(btn_label, "Press me!")                   ;
    lv_obj_center(btn_label)                                    ;   
}

//---------------------------инициализация UI---------------------------//
void gui_init(void)         {
    ESP_LOGI(TAG, "Initializing GUI...");
    run_demo_UI ();
    ESP_LOGI(TAG, "GUI initialized");
}

//------------------------------------------------------------------------//
void gui_tick(void)
{
    // Обновление UI (время, статусы)
}






