idf_component_register(	SRCS "main.c" 
						PRIV_REQUIRES esp_wifi nvs_flash
						INCLUDE_DIRS "."
						REQUIRES
						esp_driver_gpio 
						lvgl 
						driver 
						net 
						gui_demo 
						esp_lcd_st7796
						esp_lvgl_port
						)

