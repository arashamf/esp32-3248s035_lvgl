# Регистрируем компонент 'net' в системе сборки
idf_component_register(SRCS "ntp.c" "udp.c" "wifi.c"
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash esp_netif esp_event wifi_provisioning lwip driver)
                       