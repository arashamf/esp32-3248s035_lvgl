#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------//
extern lv_display_t *lvgl_disp      ;
extern lv_indev_t *lvgl_touch_indev ;

//--------------------------------------------------------------//
esp_err_t init_lcd(void)    ;
esp_err_t init_lvgl(void)   ;
void run_display(void)      ;
bool is_lvgl_ready(void)    ;
esp_err_t switch_lcd_backlight(bool backlight_level);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H