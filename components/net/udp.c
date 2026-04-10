#include "udp.h"
//----------------------------------------------------------------------//
static const char *TAG = "udp";

//----------------------------------------------------------------------//
void udp_task(void *pvParameters) {
  TickType_t pxPreviousWakeTime;
  int sockfd;
  struct sockaddr_in servaddr; //сокет для подключаемого сервера
  struct sockaddr_in cliaddr; //сокет для клиента 
  ESP_LOGI(TAG, "Create socket...\n");
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0 ) { //AF_INET - IPv4, SOCK_DGRAM - UDP,
    ESP_LOGE(TAG, "socket not created\n");
    vTaskDelete(NULL);
  }
  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));
  //-------------------Заполнение информации о клиенте-------------------//
  cliaddr.sin_family      =   AF_INET;      
  cliaddr.sin_addr.s_addr =   INADDR_ANY;     //members are in network byte order
  cliaddr.sin_port        =   htons(CONFIG_CLIENT_PORT);
  if (bind(sockfd, (const struct sockaddr *)&cliaddr,  sizeof(struct sockaddr_in)) < 0 )  { 
    ESP_LOGE(TAG, "socket not binded\n"); 
    vTaskDelete(NULL);  
  }
  else {  ESP_LOGI(TAG, "socket was binded\n"); }
  //------------Заполнение информации о подключаемом сервере------------//
  servaddr.sin_family       =   AF_INET; // IPv4
  servaddr.sin_addr.s_addr  =   inet_addr(CONFIG_SERVER_IP);
  servaddr.sin_port         =   htons(CONFIG_SERVER_PORT); 
  pxPreviousWakeTime = xTaskGetTickCount();
  for(uint16_t i=0;;i++) {
    sendto(sockfd, &i, 2,  0, (struct sockaddr*) &servaddr,  sizeof(servaddr));
    vTaskDelayUntil( &pxPreviousWakeTime, ( 1000 / portTICK_PERIOD_MS ) );
  }
  shutdown(sockfd, 0);
  close(sockfd);
  vTaskDelete(NULL);
}

//----------------------------------------------------------------------//


