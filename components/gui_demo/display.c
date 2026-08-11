#include "display.h"

#include "esp_log.h"
#include "esp_check.h"

#include "driver/i2c.h"
//#include "spi_bus.h"
#include "driver/spi_master.h"

#include "esp_lcd_st7796.h"

#if CONFIG_TOUCH_ENABLE
#include "esp_lcd_touch_gt911.h"
#endif

#include "lvgl.h"
#include "esp_lvgl_port.h"

const char *TAG = "DISPLAY";

// LCD pin assignment
#define LCD_PANEL_SCK (CONFIG_LCD_SCLK_GPIO)
#define LCD_PANEL_MISO (CONFIG_LCD_MISO_GPIO)
#define LCD_PANEL_MOSI (CONFIG_LCD_MOSI_GPIO)
#define LCD_PANEL_CS (CONFIG_LCD_CS_GPIO)
#define LCD_PANEL_DC (CONFIG_LCD_DC_GPIO)
#define LCD_PANEL_BL (CONFIG_LCD_BL_GPIO)
#define LCD_PANEL_RST (CONFIG_LCD_RST_GPIO)

// Touch pin assignment
#define TOUCH_SCL_GPIO (CONFIG_TOUCH_PANEL_SCL_GPIO)
#define TOUCH_SDA_GPIO (CONFIG_TOUCH_PANEL_SDA_GPIO)
#define TOUCH_IRQ_GPIO (CONFIG_TOUCH_PANEL_IRQ_GPIO)
#define TOUCH_RST_GPIO (CONFIG_TOUCH_PANEL_RST_GPIO)

// LCD settings
#define LCD_PANEL_WIDTH CONFIG_LCD_WIDTH
#define LCD_PANEL_HEIGHT CONFIG_LCD_HEIGHT
#define LCD_PANEL_HOST (SPI2_HOST)
#define LCD_PANEL_PIXEL_CLK_HZ (40 * 1000 * 1000)
#define LCD_PANEL_CMD_BITS (8)
#define LCD_PANEL_PARAM_BITS (8)
#define LCD_PANEL_COLOR_SPACE (ESP_LCD_COLOR_SPACE_BGR)
#define LCD_PANEL_BITS_PER_PIXEL (16)
#define LCD_PANEL_DRAW_BUFF_DOUBLE (0)
#define LCD_PANEL_DRAW_BUFF_HEIGHT (20)
#define LCD_PANEL_BL_ON_LEVEL (1)

// Touch settings
#define TOUCH_I2C_CLK_HZ (400 * 1000)

static esp_lcd_panel_io_handle_t lcd_panel_io_handle;
static esp_lcd_panel_handle_t lcd_panel_handle;
static bool lvgl_initialized = false;

esp_err_t init_lcd(void)
{
    esp_err_t ret = ESP_OK;

    gpio_config_t backlight_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PANEL_BL,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&backlight_gpio_config), TAG, "Failed to ini LCD backlight");

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t bus_config = {
        .sclk_io_num = LCD_PANEL_SCK,
        .mosi_io_num = LCD_PANEL_MOSI,
        .miso_io_num = LCD_PANEL_MISO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_PANEL_WIDTH * LCD_PANEL_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_PANEL_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "SPI initialization failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PANEL_DC,
        .cs_gpio_num = LCD_PANEL_CS,
        .pclk_hz = LCD_PANEL_PIXEL_CLK_HZ,
        .lcd_cmd_bits = LCD_PANEL_CMD_BITS,
        .lcd_param_bits = LCD_PANEL_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_PANEL_HOST, &io_config, &lcd_panel_io_handle), err, TAG, "Failed to create LCD panel");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PANEL_RST,
        .color_space = LCD_PANEL_COLOR_SPACE,
        .bits_per_pixel = LCD_PANEL_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7796(lcd_panel_io_handle, &panel_config, &lcd_panel_handle), err, TAG, "Failed to create ST7796 panel");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(lcd_panel_handle), TAG, "Unable to reset the LCD panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(lcd_panel_handle), TAG, "Unable to initialize LCD panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(lcd_panel_handle, true, true), TAG, "Unable to mirroring the LCD panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(lcd_panel_handle, true), TAG, "Unable to turn on the LCD panel");
    return gpio_set_level(LCD_PANEL_BL, LCD_PANEL_BL_ON_LEVEL);

err:
    if (lcd_panel_handle)       {   esp_lcd_panel_del(lcd_panel_handle);    }
    if (lcd_panel_io_handle)    {   esp_lcd_panel_io_del(lcd_panel_io_handle);  }
    spi_bus_free(LCD_PANEL_HOST);

    return ret;
}

#if CONFIG_TOUCH_ENABLE
//static esp_lcd_touch_handle_t lcd_touch_handle;
esp_err_t init_touch(void)
{
    ESP_LOGI(TAG, "Initializing touch panel GT911...");
    ESP_LOGI(TAG, "Touch pins - SCL:%d SDA:%d RST:%d IRQ:%d", 
             TOUCH_SCL_GPIO, TOUCH_SDA_GPIO, TOUCH_RST_GPIO, TOUCH_IRQ_GPIO);
    
    // Инициализация I2C
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_SDA_GPIO,
        .scl_io_num = TOUCH_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TOUCH_I2C_CLK_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM_0, &i2c_conf), TAG, "I2C configuration failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0), TAG, "I2C initialization failed");

    ESP_LOGI(TAG, "I2C bus initialized successfully");

    // Настройка пина сброса тач-панели
    if (TOUCH_RST_GPIO != GPIO_NUM_NC) {
        gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << TOUCH_RST_GPIO,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&rst_gpio_config), TAG, "Failed to init touch RST");
        
        // Сброс тач-панели
        gpio_set_level(TOUCH_RST_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(TOUCH_RST_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Настройка пина прерывания тач-панели
    if (TOUCH_IRQ_GPIO != GPIO_NUM_NC) {
        gpio_config_t irq_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = 1ULL << TOUCH_IRQ_GPIO,
            .pull_up_en = GPIO_PULLUP_ENABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&irq_gpio_config), TAG, "Failed to init touch IRQ");
    }

    // Создание I2C панели для тач-контроллера
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &tp_io_handle), TAG, "New touch panel IO failed");

    // Настройка тач-панели GT911
    const esp_lcd_touch_config_t tp_config = {
        .x_max = LCD_PANEL_WIDTH,
        .y_max = LCD_PANEL_HEIGHT,
        .rst_gpio_num = TOUCH_RST_GPIO,
        .int_gpio_num = TOUCH_IRQ_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_config, &lcd_touch_handle), TAG, "Failed to create GT911 touch");
    
    ESP_LOGI(TAG, "Touch panel GT911 initialized successfully");
    return ESP_OK;
}
#endif

lv_display_t *lvgl_disp = NULL;
lv_indev_t *lvgl_touch_indev = NULL;
esp_err_t init_lvgl(void)
{
    lvgl_port_cfg_t lvgl_config = {
        .task_priority = 4,
        .task_stack = 8 * 1024,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 10,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_config), TAG, "LVGL port initialization failed");

    ESP_LOGD(TAG, "Add LCD screen");
    lvgl_port_display_cfg_t disp_config = {
        .io_handle = lcd_panel_io_handle,
        .panel_handle = lcd_panel_handle,
        .buffer_size = LCD_PANEL_WIDTH * LCD_PANEL_DRAW_BUFF_HEIGHT,
        .double_buffer = LCD_PANEL_DRAW_BUFF_DOUBLE,
        .hres = LCD_PANEL_WIDTH,
        .vres = LCD_PANEL_HEIGHT,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,     // Отражение по горизонтали
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    lvgl_disp = lvgl_port_add_disp(&disp_config);
    if (lvgl_disp == NULL) {
        ESP_LOGE(TAG, "Failed to add display to LVGL");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "Display added to LVGL");

    #if CONFIG_TOUCH_ENABLE
    const lvgl_port_touch_cfg_t touch_config = {
        .disp = lvgl_disp,
        .handle = lcd_touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_config);
    #endif

    lvgl_initialized = true;
    return ESP_OK;
}

static uint32_t backlight_level = LCD_PANEL_BL_ON_LEVEL;
esp_err_t switch_lcd_backlight(bool backlight_level)
{
    return gpio_set_level(LCD_PANEL_BL, backlight_level);
}

void run_display(void)
{
    ESP_ERROR_CHECK(init_lcd());
    #if CONFIG_TOUCH_ENABLE
    ESP_ERROR_CHECK(init_touch());
    #endif
    ESP_ERROR_CHECK(init_lvgl());
}

bool is_lvgl_ready(void)
{   return lvgl_initialized;    }

