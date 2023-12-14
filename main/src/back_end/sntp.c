#include "sntp.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "status_routine.h"

static esp_sntp_config_t config_sntp=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
void SNTP_init(){
    esp_netif_sntp_init(&config_sntp);
}

void SNTP_Update(){
    esp_netif_sntp_start();
}

struct tm SNTP_GetTime(){
    time_t rawtime;
    time(&rawtime);
    struct tm timeinfo=*localtime(&rawtime);
    return timeinfo;
}