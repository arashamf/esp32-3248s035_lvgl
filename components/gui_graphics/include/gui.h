#ifndef GUI_H
#define GUI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------C-совместимые функции (вызываются из main)-------------//
void gui_init(void)     ;
void gui_tick(void)     ;

#ifdef __cplusplus
}
#endif

#endif // GUI_H
