#ifndef _STATUS_ROUTINE_
#define _STATUS_ROUTINE_

#ifdef __cplusplus
extern "C"{
#endif

#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "lvgl.h"

/*
这个头文件包含一个一直存在
用于显示各种状态(e.g.连接wifi时弹出提示连接成功or失败)的任务
和一个储存各种控制信息的数据结构
和一些标志位的宏定义
可能还会包括一些杂项
*/

#define ROUTINE_BIT_WIFI_SCANNING BIT0
#define ROUTINE_BIT_WIFI_CONNECTED BIT1//更新主界面状态栏

typedef struct
{   
    //esp system
    bool wifi;
    //lvgl labels in status bar
    lv_obj_t* label_wifiStatus;
    //lvgl menu
    bool menu_wifi_switch;

    
} Common_status;

extern EventGroupHandle_t eventGroup_routine; 
extern Common_status common_status;

void Task_Routine(void * arg);

#ifdef __cplusplus
}
#endif

#endif