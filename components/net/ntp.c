//------------------------------------------------------------------------------------------------//
#include "ntp.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

//------------------------------------------------------------------------------------------------//
#define MAX_RETRY           20
#define INET6_ADDRSTRLEN    48

//------------------------------------------------------------------------------------------------//
static const char *TAG = "example";
char strftime_buf[64];
char buff[INET6_ADDRSTRLEN];

//------------------------------------------------------------------------------------------------//
void time_sync_notification_cb(struct timeval *tv);
static void print_servers(void);
//------------------------------------------------------------------------------------------------//
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    // Установка часового пояса (например, Москва)
    setenv("TZ", "MSK-3", 1); // Внимание: формат может отличаться от стандартного POSIX
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);  // Режимы работы SNTP: по умолчанию используется одноадресный опрос. Режим должен быть установлен перед вызовом sntp_init()
    esp_sntp_setservername(0, "0.ru.pool.ntp.org");
    esp_sntp_setservername(1, "1.ru.pool.ntp.org");
    esp_sntp_setservername(2, "ntp0.NL.net");
    esp_sntp_setservername(3, "ntp2.vniiftri.ru");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb); // (Опционально) Установка коллбэка на событие синхронизации
    esp_sntp_init();     // Запуск SNTP
    print_servers();
}


//--------------------------------------------------------------------------------------------------------//
void ntp_task(void *pvParameters) {
    initialize_sntp();  // Сначала инициализируем SNTP
    // Ожидание синхронизации
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    while (1) {
        while ((esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT) && (++retry < MAX_RETRY))   {
            ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, MAX_RETRY);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        time(&now); // Получаем текущее время
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "TIME:%d:%d:%d", timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);  
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

//--------------------------------------------------------------------------------------------------------//
static void print_servers(void) {
    ESP_LOGI(TAG, "List of configured NTP servers:");
    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i)  {
        if (esp_sntp_getservername(i))  {   //Gets SNTP server name
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } 
        else    {
            // we have either IPv4 or IPv6 address, let's print it
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)  {   ESP_LOGI(TAG, "server %d: %s", i, buff);    }
        }
    }
}

//--------------------------------------------------------------------------------------------------------//
void time_sync_notification_cb(struct timeval *tv)  {
    struct tm timeinfo;
    char strftime_buf[20];
    localtime_r(&tv->tv_sec, &timeinfo);
    if (timeinfo.tm_year < (1970 - 1900)) {
        ESP_LOGI(TAG, "Time synchronization failed!");
    } 
    else {
    // Post time event
    // eventLoopPost(RE_TIME_EVENTS, RE_TIME_SNTP_SYNC_OK, nullptr, 0, portMAX_DELAY);
    strftime(strftime_buf, sizeof(strftime_buf), "%d.%m.%Y %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Time synchronization completed, current time: %s", strftime_buf);
  };
}