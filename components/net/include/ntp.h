#ifndef MAIN_NTP_H_
#define MAIN_NTP_H_

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------//
void initialize_sntp(void);
void get_ntp_time(void);
void ntp_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_NTP_H_ */