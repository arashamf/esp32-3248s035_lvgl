#ifndef I2C_BB_H
#define I2C_BB_H

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------Пины I2C 
#define IIC_SCL_GPIO    CONFIG_TOUCH_SCL_GPIO
#define IIC_SDA_GPIO    CONFIG_TOUCH_SDA_GPIO
#define IIC_RST_GPIO    CONFIG_TOUCH_RST_GPIO

//----------------------------Функции I2C
void    i2c_bb_rst          (uint8_t val)   ;
void    i2c_bb_init         (void)          ;
void    i2c_bb_start        (void)          ;
void    i2c_bb_stop         (void)          ;
uint8_t i2c_bb_wait_ack     (void)          ;
void    i2c_bb_ack          (void)          ;
void    i2c_bb_nack         (void)          ;
void    i2c_bb_send_byte    (uint8_t txd)   ;
uint8_t i2c_bb_read_byte    (uint8_t ack)   ;

#ifdef __cplusplus
}
#endif

#endif // I2C_BB_H