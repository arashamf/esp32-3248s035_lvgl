#include "lvgl.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7796.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#if CONFIG_TOUCH_ENABLE
#include "esp_lcd_touch.h"
#include "gt911.h"
#include "i2c_bb.h"  
#include "display.h"
#endif

const char *TAG = "DISPLAY";

// LCD pin assignment
#define LCD_PANEL_SCK   (CONFIG_LCD_SCLK_GPIO)
#define LCD_PANEL_MISO  (CONFIG_LCD_MISO_GPIO)
#define LCD_PANEL_MOSI  (CONFIG_LCD_MOSI_GPIO)
#define LCD_PANEL_CS    (CONFIG_LCD_CS_GPIO)
#define LCD_PANEL_DC    (CONFIG_LCD_DC_GPIO)
#define LCD_PANEL_BL    (CONFIG_LCD_BL_GPIO)
#define LCD_PANEL_RST   (CONFIG_LCD_RST_GPIO)

// Touch pin assignment
#define TOUCH_SCL_GPIO  (CONFIG_TOUCH_SCL_GPIO) 
#define TOUCH_SDA_GPIO  (CONFIG_TOUCH_SDA_GPIO)
#define TOUCH_INT_GPIO  (CONFIG_TOUCH_IRQ_GPIO)
#define TOUCH_RST_GPIO  (CONFIG_TOUCH_RST_GPIO)

// LCD settings
#define LCD_PANEL_WIDTH             CONFIG_LCD_WIDTH
#define LCD_PANEL_HEIGHT            CONFIG_LCD_HEIGHT
#define LCD_PANEL_HOST              (SPI2_HOST)
#define LCD_PANEL_PIXEL_CLK_HZ      (40 * 1000 * 1000)
#define LCD_PANEL_CMD_BITS          (8)
#define LCD_PANEL_PARAM_BITS        (8)
#define LCD_PANEL_COLOR_SPACE       (ESP_LCD_COLOR_SPACE_BGR)
#define LCD_PANEL_BITS_PER_PIXEL    (16)
#define LCD_PANEL_DRAW_BUFF_DOUBLE  (0)
#define LCD_PANEL_DRAW_BUFF_HEIGHT  (20)
#define LCD_PANEL_BL_ON_LEVEL       (1)
// Touch settings
#define TOUCH_I2C_CLK_HZ (400 * 1000)

//-------------------------------------------------------------------------------------------//
static bool                         lvgl_initialized    = false ;
lv_indev_t                          *lvgl_touch_indev   = NULL  ;
lv_display_t                        *lvgl_disp          = NULL  ;                   
static esp_lcd_panel_io_handle_t    lcd_panel_io_handle         ;
static esp_lcd_panel_handle_t       lcd_panel_handle            ;
static uint32_t backlight_level =   LCD_PANEL_BL_ON_LEVEL       ;

//-------------------------------------------------------------------------------------------//
#if CONFIG_TOUCH_ENABLE
static void touchpad_read   (lv_indev_t *indev, lv_indev_data_t *data);
static esp_err_t init_touch (void);

//-------------------------------------------------------------------------------------------//
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)     {
    uint16_t x, y;
    uint8_t touched = 0;
    static uint8_t last_touched = 0;

    // Читаем данные из GT911
    gt911_read_touch(&x, &y, &touched);
    
    if (touched) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        if (!last_touched)  { ESP_LOGI(TAG, "Touch START: x=%d, y=%d", x, y);   } 
        else                {  ESP_LOGD(TAG, "Touch MOVE: x=%d, y=%d", x, y);   }
    } 
    else {  
        data->state = LV_INDEV_STATE_RELEASED;  
        if (last_touched) { ESP_LOGI(TAG, "Touch END"); }
    }
    last_touched = touched;
}

//-------------------------------------------------------------------------------------------//
static esp_err_t init_touch(void)
{
    // 1. Инициализация контроллера GT911
    ESP_LOGI(TAG, "Initializing GT911 touch panel...");
    ESP_RETURN_ON_ERROR(gt911_init(), TAG, "GT911 init failed");
    // 2. Регистрация устройства ввода в LVGL (новый API v9)
    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL input device");
        return ESP_ERR_NO_MEM;
    }
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);

    ESP_LOGI(TAG, "Touch registered in LVGL");
    return ESP_OK;
}

#endif

//-------------------------------------------------------------------------------------------//
esp_err_t init_lcd(void)        {
    esp_err_t ret = ESP_OK;

    gpio_config_t backlight_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PANEL_BL,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&backlight_gpio_config), TAG, "Failed to ini LCD backlight");

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t bus_config = {
        .sclk_io_num    = LCD_PANEL_SCK     ,
        .mosi_io_num    = LCD_PANEL_MOSI    ,
        .miso_io_num    = LCD_PANEL_MISO    ,
        .quadwp_io_num  = GPIO_NUM_NC       ,
        .quadhd_io_num  = GPIO_NUM_NC       ,
        .max_transfer_sz = LCD_PANEL_WIDTH * LCD_PANEL_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_PANEL_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "SPI initialization failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num    = LCD_PANEL_DC,
        .cs_gpio_num    = LCD_PANEL_CS,
        .pclk_hz        = LCD_PANEL_PIXEL_CLK_HZ,
        .lcd_cmd_bits   = LCD_PANEL_CMD_BITS,
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
    if (lcd_panel_handle)       {   esp_lcd_panel_del(lcd_panel_handle);        }
    if (lcd_panel_io_handle)    {   esp_lcd_panel_io_del(lcd_panel_io_handle);  }
    spi_bus_free(LCD_PANEL_HOST);

    return ret;
}

//-------------------------------------------------------------------------------------------//
esp_err_t init_lvgl(void)   {
    //--------------1. ИНИЦИАЛИЗАЦИЯ LVGL ПОРТА
    lvgl_port_cfg_t lvgl_config = {
        .task_priority = 4,
        .task_stack = 8 * 1024,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 10,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_config), TAG, "LVGL port initialization failed");
    //------------ 2. ДОБАВЛЕНИЕ ДИСПЛЕЯ 
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

     //-----------3. ДОБАВЛЕНИЕ ТАЧ-ПАНЕЛИ 
    /*#if CONFIG_TOUCH_ENABLE
    ESP_LOGI(TAG, "Initializing GT911 touch...");
    ESP_RETURN_ON_ERROR(gt911_init(), TAG, "GT911 init failed");
    #endif*/

    lvgl_initialized = true;
    return ESP_OK;
}

//-------------------------------------------------------------------------------------------//
esp_err_t switch_lcd_backlight(bool backlight_level)    {   
    return gpio_set_level(LCD_PANEL_BL, backlight_level);   
}

//-------------------------------------------------------------------------------------------//
void run_display(void)      {
    ESP_ERROR_CHECK(init_lcd());
    ESP_ERROR_CHECK(init_lvgl());
    #if CONFIG_TOUCH_ENABLE
    ESP_ERROR_CHECK(init_touch());
    #endif
}

//-------------------------------------------------------------------------------------------//
bool is_lvgl_ready(void)    {   return lvgl_initialized;    }


