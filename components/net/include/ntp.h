#ifndef MAIN_NTP_H_
#define MAIN_NTP_H_
//-------------------------------------------------------------//
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "esp_sleep.h"

//-------------------------------------------------------------//
void initialize_sntp(void);
void get_ntp_time(void);
void ntp_task(void *pvParameters);
//-------------------------------------------------------------//
#endif /* MAIN_NTP_H_ */