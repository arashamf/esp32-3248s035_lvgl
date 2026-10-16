idf_component_register(	
						SRCS 
						"main.c" 
						INCLUDE_DIRS "."
						PRIV_REQUIRES 
						esp_wifi 
						nvs_flash
						REQUIRES
						esp_driver_gpio 
						lvgl 
						driver 
						net 
						led
						gui_graphics
						esp_lcd_st7796
						esp_lvgl_port
						touch_drivers
						)

