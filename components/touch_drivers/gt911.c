#include "gt911.h"
#include "i2c_bb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CT_MAX_TOUCH        5  

//----------------------------------------------------------------------------//
typedef struct
{
  uint8_t Touch;
  uint8_t TouchpointFlag;
  uint8_t TouchCount;

  uint8_t Touchkeytrackid   [CT_MAX_TOUCH];
  uint16_t X                [CT_MAX_TOUCH];
  uint16_t Y                [CT_MAX_TOUCH];
  uint16_t S                [CT_MAX_TOUCH];
} GT911_Dev;

//----------------------------------------------------------------------------//
static const char *TAG = "GT911";

static uint8_t s_GT911_CfgParams[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GT9111_CFG_TBL[] =
{
  0X60, 0X40, 0X01, 0XE0, 0X01, 0X05, 0X35, 0X00, 0X02, 0X08,
  0X1E, 0X08, 0X50, 0X3C, 0X0F, 0X05, 0X00, 0X00, 0XFF, 0X67,
  0X50, 0X00, 0X00, 0X18, 0X1A, 0X1E, 0X14, 0X89, 0X28, 0X0A,
  0X30, 0X2E, 0XBB, 0X0A, 0X03, 0X00, 0X00, 0X02, 0X33, 0X1D,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X32, 0X00, 0X00,
  0X2A, 0X1C, 0X5A, 0X94, 0XC5, 0X02, 0X07, 0X00, 0X00, 0X00,
  0XB5, 0X1F, 0X00, 0X90, 0X28, 0X00, 0X77, 0X32, 0X00, 0X62,
  0X3F, 0X00, 0X52, 0X50, 0X00, 0X52, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0F,
  0X0F, 0X03, 0X06, 0X10, 0X42, 0XF8, 0X0F, 0X14, 0X00, 0X00,
  0X00, 0X00, 0X1A, 0X18, 0X16, 0X14, 0X12, 0X10, 0X0E, 0X0C,
  0X0A, 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X29, 0X28, 0X24, 0X22, 0X20, 0X1F, 0X1E, 0X1D,
  0X0E, 0X0C, 0X0A, 0X08, 0X06, 0X05, 0X04, 0X02, 0X00, 0XFF,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF,
  0XFF, 0XFF, 0XFF, 0XFF,
};
static GT911_Dev Dev_Now;
static GT911_Dev Dev_Backup;
static bool touched = 0;

//----------------------------------------------------------------------//
static void     i2c_bb_write_reg    (uint16_t reg, uint8_t *data, size_t len)   ;
static void     i2c_bb_read_reg     (uint16_t reg, uint8_t *data, size_t len)   ;
static void     gt911_reset         (void)                                      ;
static uint8_t  gt911_send_cfg      (uint8_t mode)                              ;
static uint8_t  gt911_read_status   (void)                                      ;
static void     gt911_scan          (void)                                      ;

//------------------------запись массива байта------------------------//
static void i2c_bb_write_reg(uint16_t reg, uint8_t *data, size_t len)  {
    i2c_bb_start();
    i2c_bb_send_byte(GT_CMD_WR)         ;   // Адрес GT911 (запись)
    i2c_bb_wait_ack()                   ;
    i2c_bb_send_byte(reg >> 8)          ;
    i2c_bb_wait_ack()                   ;
    i2c_bb_send_byte(reg & 0xFF)        ;
    i2c_bb_wait_ack()                   ;
    for (size_t i = 0; i < len; i++)    {
        i2c_bb_send_byte(data[i])       ;
        i2c_bb_wait_ack()               ;
    }
    i2c_bb_stop();
}

//------------------------чтение массива байта------------------------//
static void i2c_bb_read_reg(uint16_t reg, uint8_t *data, size_t len)   {
    i2c_bb_start()                      ;
    i2c_bb_send_byte(GT_CMD_WR)         ;   //aдрес GT911 (запись)
    i2c_bb_wait_ack()                   ;
    i2c_bb_send_byte(reg >> 8)          ;
    i2c_bb_wait_ack()                   ;
    i2c_bb_send_byte(reg & 0xFF)        ;
    i2c_bb_wait_ack()                   ;
    
    i2c_bb_start();
    i2c_bb_send_byte(GT_CMD_RD)         ;   // Адрес GT911 (чтение)
    i2c_bb_wait_ack()                   ;
    for (size_t i = 0; i < len; i++)    {
        data[i] = i2c_bb_read_byte(i < len - 1);
    }
    i2c_bb_stop();
}

//-----------------------------Сброс GT911-----------------------------//
static void gt911_reset(void)       {
    i2c_bb_rst(0);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_bb_rst(1);
    vTaskDelay(pdMS_TO_TICKS(200));
}


//-----отправка команды обновления конфигурации в контроллер GT911-----//
static uint8_t gt911_send_cfg(uint8_t mode)     {
    uint8_t buf[2];
    buf[0] = 0;
    buf[1] = mode;
    i2c_bb_write_reg(GT_CHECK_REG, buf, 2);
    return 0;
}

/**/
//------------------чтение ID чипа (регистр 0x8140)------------------//
static uint8_t gt911_read_status(void)       {
    uint8_t buf[4];
    i2c_bb_read_reg(GT_PID_REG,buf,3);
    i2c_bb_read_reg(GT_CFGS_REG,&buf[3],1);
    ESP_LOGI(TAG,"TouchPad_ID: %d,%d,%d", buf[0], buf[1], buf[2]);
    ESP_LOGI(TAG,"TouchPad_Config_Version: %02X", buf[3]);
    return buf[3];
}

//------------чтение координат касаний из контроллера GT911------------//
static void gt911_scan(void)    {
    uint8_t buf[64];
    uint8_t Clearbuf = 0;
    uint8_t i;

    Dev_Now.Touch = 0;
    i2c_bb_read_reg(GT_READ_REG, buf, 1);

    if ((buf[0] & 0x80) == 0x00) {
        touched = 0;
        i2c_bb_write_reg(GT_READ_REG, &Clearbuf, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    } 
    else {
        touched = 1;
        Dev_Now.TouchpointFlag = buf[0];
        Dev_Now.TouchCount = buf[0] & 0x0F;

        if (Dev_Now.TouchCount > CT_MAX_TOUCH)     {
            touched = 0;
            i2c_bb_write_reg(GT_READ_REG, &Clearbuf, 1);
            return;
        }

        i2c_bb_read_reg(GT_READ_REG + 1, &buf[1], Dev_Now.TouchCount * 8);
        i2c_bb_write_reg(GT_READ_REG, &Clearbuf, 1);

        Dev_Now.Touchkeytrackid[0] = buf[1];
        Dev_Now.X[0] = ((uint16_t)buf[3] << 8) + buf[2];
        Dev_Now.Y[0] = ((uint16_t)buf[5] << 8) + buf[4];
        Dev_Now.S[0] = ((uint16_t)buf[7] << 8) + buf[6];

        Dev_Now.Touchkeytrackid[1] = buf[9];
        Dev_Now.X[1] = ((uint16_t)buf[11] << 8) + buf[10];
        Dev_Now.Y[1] = ((uint16_t)buf[13] << 8) + buf[12];
        Dev_Now.S[1] = ((uint16_t)buf[15] << 8) + buf[14];

        Dev_Now.Touchkeytrackid[2] = buf[17];
        Dev_Now.X[2] = ((uint16_t)buf[19] << 8) + buf[18];
        Dev_Now.Y[2] = ((uint16_t)buf[21] << 8) + buf[20];
        Dev_Now.S[2] = ((uint16_t)buf[23] << 8) + buf[22];

        Dev_Now.Touchkeytrackid[3] = buf[25];
        Dev_Now.X[3] = ((uint16_t)buf[27] << 8) + buf[26];
        Dev_Now.Y[3] = ((uint16_t)buf[29] << 8) + buf[28];
        Dev_Now.S[3] = ((uint16_t)buf[31] << 8) + buf[30];

        Dev_Now.Touchkeytrackid[4] = buf[33];
        Dev_Now.X[4] = ((uint16_t)buf[35] << 8) + buf[34];
        Dev_Now.Y[4] = ((uint16_t)buf[37] << 8) + buf[36];
        Dev_Now.S[4] = ((uint16_t)buf[39] << 8) + buf[38];

        for (i = 0; i < Dev_Now.TouchCount; i++)            {
            if (Dev_Now.X[i] > GT911_MAX_WIDTH)     {   Dev_Now.X[i] = GT911_MAX_WIDTH;     }
            if (Dev_Now.Y[i] > GT911_MAX_HEIGHT)    {   Dev_Now.Y[i] = GT911_MAX_HEIGHT;    }
        }

        if (Dev_Now.TouchCount == 0) {  touched = 0;    }
    }
}


//-------------------Инициализация контроллера gt911-------------------//
esp_err_t gt911_init(void)              {
    uint8_t config_Checksum = 0, i;

    ESP_LOGI(TAG, "Initializing GT911...");
    // 1. Инициализация I2C
    i2c_bb_init();

    // 2. Сброс GT911
    gt911_reset();

    // 3. Чтение конфигурации
    i2c_bb_read_reg(GT_CFGS_REG, (uint8_t *)&s_GT911_CfgParams[0], 186);

    // 4. Проверка контрольной суммы
    for (i = 0; i < sizeof(s_GT911_CfgParams) - 2; i++) {
        config_Checksum += s_GT911_CfgParams[i];
    }

    ESP_LOGI(TAG, "Config checksum: 0x%02X, expected: 0x%02X",
             s_GT911_CfgParams[184], ((~config_Checksum) + 1) & 0xFF);

    if (s_GT911_CfgParams[184] == (((~config_Checksum) + 1) & 0xFF))    {
        ESP_LOGI(TAG, "READ CONFIG SUCCESS!");
        ESP_LOGI(TAG, "%dx%d",
                 s_GT911_CfgParams[2] << 8 | s_GT911_CfgParams[1],
                 s_GT911_CfgParams[4] << 8 | s_GT911_CfgParams[3]);

        if ((GT911_MAX_WIDTH != (s_GT911_CfgParams[2] << 8 | s_GT911_CfgParams[1])) ||
            (GT911_MAX_HEIGHT != (s_GT911_CfgParams[4] << 8 | s_GT911_CfgParams[3]))) {

            s_GT911_CfgParams[1] = GT911_MAX_WIDTH & 0xFF;
            s_GT911_CfgParams[2] = GT911_MAX_WIDTH >> 8;
            s_GT911_CfgParams[3] = GT911_MAX_HEIGHT & 0xFF;
            s_GT911_CfgParams[4] = GT911_MAX_HEIGHT >> 8;
            s_GT911_CfgParams[185] = 1;

            config_Checksum = 0;
            for (i = 0; i < sizeof(s_GT911_CfgParams) - 2; i++) {
                config_Checksum += s_GT911_CfgParams[i];
            }
            s_GT911_CfgParams[184] = (~config_Checksum) + 1;

            ESP_LOGI(TAG, "Updating config to %dx%d", GT911_MAX_WIDTH, GT911_MAX_HEIGHT);

            i2c_bb_write_reg(GT_CFGS_REG, (uint8_t *)s_GT911_CfgParams, sizeof(s_GT911_CfgParams));
            i2c_bb_read_reg(GT_CFGS_REG, (uint8_t *)&s_GT911_CfgParams[0], 186);
        }
    }
    // 5. Чтение ID и версии
    gt911_read_status();
    ESP_LOGI(TAG, "GT911 initialized successfully");
    return ESP_OK;
}

//----------------------проверка наличия касаний----------------------//
esp_err_t gt911_read_touch(uint16_t *x, uint16_t *y, uint8_t *touched_out)      {
    gt911_scan();   // обновляет static touched и Dev_Now
    if (touched)        {
        *x = Dev_Now.X[0];
        *y = Dev_Now.Y[0];
        *touched_out = 1;
    } 
    else {  *touched_out = 0;   }
    return ESP_OK;
}


