#include "wifi.h"
//--------------------------------------------------------------------------
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

//--------------------------------------------------------------------------
#define     WIFI_INITIALIZED_BIT    BIT0    //инициализация WiFi 
#define     WIFI_CONNECTED_BIT      BIT1    //соединение с WiFi сетью установлено 
#define     WIFI_FAIL_BIT           BIT2    //ошибка соединения с WiFi сетью
#define     WIFI_CONNECTING_BIT     BIT3    //бит устанавливается при попытке соединения с WiFi сетью

//--------------------------------------------------------------------------
static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_event_group = NULL; //FreeRTOS event group to signal when we are connected  
static int  s_retry_num     = 0 ;
uint16_t    ip_adress   [4]     ;
//char        ip_str      [16]    ;
static const char MAX_RETRY  =      10  ;      //максимальное количество попыток подключений перед перезагрузкой соединения
TaskHandle_t WiFiTaskHandle  =      NULL;

//--------------------------------------------------------------------------
static void print_wifi_ip (void* event_data)    ;
static void reset_wifi_event_bits   (void)      ;
static void get_wifi_status         (void)      ;
static esp_err_t wifi_reset_state   (void)      ;

//------------------------------------------------------------------------------------------------------//
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)    {
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Event group is NULL!");
        return;
    }
    //если структура для соединия к WiFi создана успешно
    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START))        {
        xEventGroupSetBits(s_wifi_event_group,WIFI_CONNECTING_BIT);
        esp_wifi_connect(); //попытка соединения
        ESP_LOGI(TAG, "WiFi start event - connecting...");
    } 
    else    {
        if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED))      {
            xEventGroupClearBits(s_wifi_event_group,WIFI_CONNECTED_BIT | WIFI_CONNECTING_BIT);
            ESP_LOGI(TAG, "Disconnected from AP"); 
            if (s_retry_num < MAX_RETRY) {  //если количество попыток соединений не исчерпано
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
                esp_wifi_connect(); //попытка соединения
                s_retry_num++;
                ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", s_retry_num, MAX_RETRY);
            } 
            else                                                                    {
                ESP_LOGE(TAG, "Failed to connect to AP after %d retries", MAX_RETRY);
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
            }
        }
        else    {
            if  ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP))     {    
                // Получен IP-адрес - подключение успешно
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
                ip_adress[0] = esp_ip4_addr1_16(&event->ip_info.ip);
                ip_adress[1] = esp_ip4_addr2_16(&event->ip_info.ip);
                ip_adress[2] = esp_ip4_addr3_16(&event->ip_info.ip);
                ip_adress[3] = esp_ip4_addr4_16(&event->ip_info.ip);
                xEventGroupClearBits(s_wifi_event_group,WIFI_FAIL_BIT | WIFI_CONNECTING_BIT);  
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);   
                s_retry_num = 0;  
            }   
        }
    }
}

//------------------------------------------------------------------------------------------------------//
static void print_wifi_ip (void* event_data)        {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
}

//------------------------------------------------------------------------------------------------------//
esp_err_t wifi_setting_init(void)                           {
    ESP_LOGI(TAG, "Initializing WiFi...");
    // Создание event group
    if (s_wifi_event_group == NULL)                             {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL)                             {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_ERR_NO_MEM;
        }    
    }
    else                                                                                {
        if ((xEventGroupGetBits(s_wifi_event_group) & WIFI_INITIALIZED_BIT) == true)        {
            ESP_LOGW(TAG, "WiFi already initialized");
            return ESP_OK;
        }
    }
     //Initialize the TCP/IP stack and the event loop
    ESP_ERROR_CHECK(esp_netif_init()); 
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta(); //User init default station (official API)
    // Инициализация WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
    xEventGroupSetBits(s_wifi_event_group,WIFI_INITIALIZED_BIT);   //установка бита "инициализирован"
     // Проверка бита WIFI_INITIALIZED_BIT
    uint32_t bits = xEventGroupGetBits(s_wifi_event_group);
    ESP_LOGI(TAG, "Current bits: 0x%08x (INIT=%d)", bits, (bits & WIFI_INITIALIZED_BIT) ? 1 : 0);
    return ESP_OK;
}

//------------------------------------------------------------------------------------------------------//
esp_err_t wifi_connect(void)        {
    // 1. Проверка инициализации битов Event Group
    if ((s_wifi_event_group == NULL) || ((xEventGroupGetBits(s_wifi_event_group) & WIFI_INITIALIZED_BIT) == false )) {
        ESP_LOGE(TAG, "WiFi not initialized. Call wifi_init() first");
        return ESP_ERR_INVALID_STATE;
    }  
    // 2. Проверка статуса подключения
    if ((xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0)  {
        ESP_LOGW(TAG, "Already connected");
        return ESP_OK;
    }
    // 3. Проверка, не идет ли уже процесс подключения
    if ((xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTING_BIT) != 0) {
        ESP_LOGW(TAG, "Connection already in progress");
        return ESP_OK;
    }
     // 4. Инициализации структуры соединия WiFi пользовательскими настройками
    wifi_config_t wifi_config = {
        .sta = {
            .ssid               = EXAMPLE_ESP_WIFI_SSID,
            .password           = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e        = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    // 5. Установка режима станции WiFi
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)              {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: 0x%04x", ret);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        return ret;
    }
    // 6.Установка конфигурации из структуры соединия WiFi
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)              {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: 0x%04x", ret);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        return ret;
    }
    // 7. Запуск WiFi, если выбран режим WIFI_MODE_STA, создается блок управления станцией и запускается станция
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: 0x%04x", ret);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        return ret;
    }
    return ESP_OK;
}

//------------------------------------------------------------------------------------------------------//
void create_wifi_task(void)             {
    xTaskCreate(wifi_task, "wifi_task", 3*1024,NULL, 4, &WiFiTaskHandle);
}

//------------------------------------------------------------------------------------------------------//
void wifi_task(void *pvParameters) {
    wifi_ap_record_t info;
    while (wifi_setting_init() != ESP_OK)                   {
        ESP_LOGE(TAG, "wifi_setting_init failed! Retrying in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    ESP_LOGI(TAG, "wifi_setting_init OK! Calling wifi_connect()...");
    esp_err_t ret = wifi_connect();
    ESP_LOGI(TAG, "wifi_connect() returned: 0x%04x", ret);

    while (1)                                           {
        if (wifi_manager_is_connected()==true)              {
            if  (esp_wifi_sta_get_ap_info(&info) == ESP_OK)     {       //информация о точке доступа, с которой связано устройство
                ESP_LOGI(TAG, "Connected to: %s,my_ip: %d.%d.%d.%d",(char *)info.ssid,
                                ip_adress[0],ip_adress[1],ip_adress[2],ip_adress[3]);
            }
        }
        else                                        {
            if (wifi_manager_has_failed() == true)      {           //если произошла ошибка 
                ESP_LOGW(TAG, "Connection failed, resetting...");
                s_retry_num = 0;
                wifi_reset_state(); 
                wifi_connect(); 
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }    
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

//------------------------------------------------------------------------------------------------------//
bool wifi_manager_is_connected(void)    {    
    if (s_wifi_event_group == NULL)         {   return false;   }
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;    
}

//-------------------------------Статус попытки соединения с WiFi сетью--------------------------------//
bool wifi_manager_is_connecting(void)   {   
    if (s_wifi_event_group == NULL)         {   return false;   }
    return (xEventGroupGetBits(s_wifi_event_group) &    WIFI_CONNECTING_BIT) != 0;   
}

//---------------------------Проверка статуса ошибки соединения с WiFi сетью---------------------------//
bool wifi_manager_has_failed(void)      {
    if (s_wifi_event_group == NULL)         {   return false;   }
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_FAIL_BIT) != 0;
}

//------------------------------------------------------------------------------------------------------//
static void get_wifi_status (void)                      {
    //-------Состояние битов статуса WiFi
    uint32_t bits = xEventGroupGetBits(s_wifi_event_group);
    ESP_LOGI(TAG, "EVENT BITS: INIT=%d, CONN=%d, CONNECTING=%d, FAIL=%d",
    (bits & WIFI_INITIALIZED_BIT) ? 1 : 0,
    (bits & WIFI_CONNECTED_BIT) ? 1 : 0,
    (bits & WIFI_CONNECTING_BIT) ? 1 : 0,
    (bits & WIFI_FAIL_BIT) ? 1 : 0);
    ESP_LOGI(TAG, "s_retry_num: %d", s_retry_num);
    //-------Статус WiFi драйвера 
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    ESP_LOGI(TAG, "WiFi mode: %d (1=STA, 2=AP, 3=STA+AP)", mode);
}

//------------------------------------------------------------------------------------------------------//
static void reset_wifi_event_bits (void)    {
    if (s_wifi_event_group == NULL)             {   return; }
    else                                            {
        xEventGroupClearBits(s_wifi_event_group,WIFI_CONNECTED_BIT | WIFI_CONNECTING_BIT | WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Forced WiFi state reset: CONN=0, CONNECTING=0, FAIL=0");
    }
}

//------------------------------------------------------------------------------------------------------//
static esp_err_t wifi_reset_state(void)     {
    reset_wifi_event_bits();
    s_retry_num = 0;
    ESP_LOGI(TAG, "WiFi state reset"); 
    esp_err_t ret;

    ret = esp_wifi_stop();
    if ((ret != ESP_OK) && (ret != ESP_ERR_WIFI_NOT_STARTED))   { 
        ESP_LOGW(TAG, "esp_wifi_stop failed: 0x%04x", ret); 
    } 
    else    {   ESP_LOGI(TAG, "esp_wifi_stop"); }
    // Отключения от AP
    ret = esp_wifi_disconnect();
    if ((ret != ESP_OK) && (ret != ESP_ERR_WIFI_NOT_CONNECT) && (ret != ESP_ERR_WIFI_NOT_STARTED))  {
        ESP_LOGW(TAG, "esp_wifi_disconnect failed: 0x%04x", ret);
    }
    else    {   ESP_LOGI(TAG, "esp_wifi_disconnect");   }
    return ret;
}


