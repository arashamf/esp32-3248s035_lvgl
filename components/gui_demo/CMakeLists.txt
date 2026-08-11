# Регистрируем компонент 'net' в системе сборки
idf_component_register(SRCS "lvgl_demo_ui.c" "images.c" "display.c" "screens.c" "styles.c" "ui.c"
                       INCLUDE_DIRS "include"
                       REQUIRES 
					   nvs_flash 
					   esp_netif 
					   esp_event 
					   wifi_provisioning 
					   lwip 
					   driver 
					   lvgl 
					   driver
					   esp_lvgl_port
					   esp_lcd_st7796
					   )
                       