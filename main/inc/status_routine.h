#ifndef _STATUS_ROUTINE_
#define _STATUS_ROUTINE_

#ifdef __cplusplus
extern "C"{
#endif

#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "lvgl.h"

/*
这个头文件包含一个一直存在
用于显示各种状态(e.g.连接wifi时弹出提示连接成功or失败)的任务
和一个储存各种控制信息的数据结构
和一些标志位的宏定义
可能还会包括一些杂项
*/
#define ROUTINE_UPDATE_NETWORK_TIME_S (60)

#define ROUTINE_BIT_WIFI_SCAN_START BIT0
#define ROUTINE_BIT_WIFI_CONNECT_SUCCESS BIT1//更新主界面状态栏
#define ROUTINE_BIT_WIFI_CONNECT_FAILED BIT2
#define ROUTINE_BIT_CITY_INPUT_INVAILD BIT3
#define ROUTINE_BIT_CITY_INPUT_SUCCESS BIT4
typedef struct
{   
    //连接wifi时才为true
    bool wifi;
    //状态栏
    lv_obj_t* obj_statusBar;
    //wifi开关，连接成功后会强制置true，但为true不一定连接wifi
    bool menu_wifi_switch;

    
} Common_status;

extern EventGroupHandle_t eventGroup_note; 
extern Common_status common_status;

extern SemaphoreHandle_t semaphoreUrlChange;

void Task_Routine(void * arg);

#ifdef __cplusplus
}
#endif

#endif