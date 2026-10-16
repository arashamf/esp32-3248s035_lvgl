idf_component_register	(
					SRCS 
						"gui.c"
						"display.c" 
						INCLUDE_DIRS 
						"include"
					REQUIRES 
						lvgl
						esp_lvgl_port
						esp_lcd_st7796
						touch_drivers
						freertos
				)