#include "lvgl.h"
#include "esp_err.h"
//--------------------------------------------------------------
extern lv_display_t *lvgl_disp;
extern lv_indev_t *lvgl_touch_indev;

//--------------------------------------------------------------
esp_err_t init_lcd(void)    ;
esp_err_t init_lvgl(void)   ;
esp_err_t switch_lcd_backlight(bool backlight_level);
void run_display(void)      ;
bool is_lvgl_ready(void)    ;

