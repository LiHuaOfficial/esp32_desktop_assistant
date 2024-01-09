#ifndef _MY_WIFI_
#define _MY_WIFI_

#ifdef __cplusplus
extern "C"{
#endif

#include "esp_system.h"



#define LVGL2WIFI_SCAN_BIT BIT0
#define LVGL2WIFI_CONNECT_BIT BIT1

#define SCAN_LIST_SIZE 10

void Task_WifiInit(void *arg);
void Task_WifiScan(void *arg);

/// @brief 断开当前连接，配置wifi config，重新连接
/// @param ssid 本来就是获取的ssid不会超出范围
/// @param pwd 通过Textarea限制长度
/// @return 
esp_err_t MyWifi_Connect(bool defaultConnect,const char* ssid,const char* pwd);
#ifdef __cplusplus
}
#endif

#endif