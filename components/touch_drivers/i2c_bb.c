#include "i2c_bb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_BB";

//--------------------------------------------------------------//
static inline void      sda_out     (void);
static inline void      sda_in      (void);
static inline void      i2c_sda     (uint8_t val);
static inline void      i2c_scl     (uint8_t val);
static inline uint8_t   read_sda    (void);

//------------------------задержка в мкс------------------------//
static inline void i2c_bb_delay_us(uint32_t us)         {
    // Для ESP32 простая задержка циклом (приблизительно)
    for (uint32_t i = 0; i < us * 80; i++) {    __asm__ __volatile__("nop");    }
}

//-----------------функции управление пинами I2C-----------------//
static inline void sda_out(void)
{   gpio_set_direction(IIC_SDA_GPIO, GPIO_MODE_OUTPUT); }

static inline void sda_in(void)
{   gpio_set_direction(IIC_SDA_GPIO, GPIO_MODE_INPUT);  }

static inline void i2c_sda(uint8_t val)
{   gpio_set_level(IIC_SDA_GPIO, val);  }

static inline void i2c_scl(uint8_t val)
{   gpio_set_level(IIC_SCL_GPIO, val);  }

static inline uint8_t read_sda(void)
{   return gpio_get_level(IIC_SDA_GPIO);    }

void i2c_bb_rst(uint8_t val)
{   gpio_set_level(IIC_RST_GPIO, val); }

//----------------------инициализация I2C----------------------//
void i2c_bb_init(void)      {
    gpio_config_t io_conf =     {
        .pin_bit_mask = (1ULL << IIC_SCL_GPIO) | (1ULL << IIC_SDA_GPIO) | (1ULL << IIC_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    i2c_sda(1);  //установка логической единицы на шине
    i2c_scl(1);    
    ESP_LOGI(TAG, "Bit-banging I2C initialized");
}

//-------------генерация start-условия I2C протокола-------------//
void i2c_bb_start(void)     {
    sda_out();
    i2c_sda(1);
    i2c_scl(1);
    i2c_bb_delay_us(4);
    i2c_sda(0);
    i2c_bb_delay_us(4);
    i2c_scl(0);
}

//-------------генерация stop-условия I2C протокола-------------//
void i2c_bb_stop(void)      {
    sda_out();
    i2c_scl(0);
    i2c_sda(0);
    i2c_bb_delay_us(4);
    i2c_scl(1);
    i2c_sda(1);
    i2c_bb_delay_us(4);
}

//-----------------ожидание получения ask-бита-----------------//
uint8_t i2c_bb_wait_ack(void)   {
    uint8_t err_time = 0;
    sda_in();
    i2c_sda(1);
    i2c_bb_delay_us(1);
    i2c_scl(1);
    i2c_bb_delay_us(1);
    
    while (read_sda()) {
        err_time++;
        if (err_time > 250) {
            i2c_bb_stop();
            return 1;
        }
    }
    i2c_scl(0);
    return 0;
}

//----------------------генерация ask-бита----------------------//
void i2c_bb_ack(void)   {
    i2c_scl(0);
    sda_out();
    i2c_sda(0);
    i2c_bb_delay_us(2);
    i2c_scl(1);
    i2c_bb_delay_us(2);
    i2c_scl(0);
}

//---------------------генерация noask-бита---------------------//
void i2c_bb_nack(void)      {
    i2c_scl(0);
    sda_out();
    i2c_sda(1);
    i2c_bb_delay_us(2);
    i2c_scl(1);
    i2c_bb_delay_us(2);
    i2c_scl(0);
}

//------------------------отправка байта------------------------//
void i2c_bb_send_byte(uint8_t txd)      {
    uint8_t t;
    sda_out();
    i2c_scl(0);
    for (t = 0; t < 8; t++) {
        if (txd & 0x80) {
            i2c_sda(1);
        } else {
            i2c_sda(0);
        }
        txd <<= 1;
        i2c_bb_delay_us(2);
        i2c_scl(1);
        i2c_bb_delay_us(2);
        i2c_scl(0);
        i2c_bb_delay_us(2);
    }
}

//-------------------------чтение байта-------------------------//
uint8_t i2c_bb_read_byte(uint8_t ack)       {
    uint8_t i, receive = 0;
    sda_in();
    for (i = 0; i < 8; i++)     {
        i2c_scl(0)              ;
        i2c_bb_delay_us(2)      ;
        i2c_scl(1)              ;
        receive <<= 1           ;
        if (read_sda()) {   receive++;  }
        i2c_bb_delay_us(1)      ;
    }
    if (!ack)   {   i2c_bb_nack();  } 
    else        {   i2c_bb_ack();   }
    return receive;
}

