#ifndef GT911_H
#define GT911_H

#include "esp_err.h"
//----------------------------------------------------------------------------//
// Регистры GT911
#define     GT_CMD_WR           (0x5D << 1 | 0)      
#define     GT_CMD_RD           (0x5D << 1 | 1)     
#define     GT911_MAX_WIDTH     320        
#define     GT911_MAX_HEIGHT    480   
#define     GT_CTRL_REG         0x8040
#define     GT_CFGS_REG         0x8047
#define     GT_CHECK_REG        0X80FF       
#define     GT_READ_REG         0x814E
#define     GT_PID_REG          0X8140      
#define     GT_GSTID_REG        0X814E                  

//----------------------------------------------------------------------------//
esp_err_t gt911_init(void);
esp_err_t gt911_read_touch(uint16_t *x, uint16_t *y, uint8_t *touched);

#endif // GT911_H