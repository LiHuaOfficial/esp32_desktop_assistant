#include "status_routine.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xGuiSemaphore;

//每当需要产生可见通知时发送bit
EventGroupHandle_t eventGroup_routine;

Common_status common_status={
    .wifi=false,
    .label_wifiStatus=NULL,
    .menu_wifi_switch=false
};

void Task_MyEventHandle(void* arg);

void Task_Routine(void *arg)
{
    xTaskCreate(Task_MyEventHandle,"MyEvent",4096,NULL,configMAX_PRIORITIES,NULL);

    //需要定时处理的操作
    

    vTaskDelete(NULL);
}

void Task_MyEventHandle(void* arg){
    //处理事件
    eventGroup_routine=xEventGroupCreate();

    while (1)
    {
        //等待
        uint32_t bits=xEventGroupWaitBits(eventGroup_routine,
                                          ROUTINE_BIT_WIFI_SCANNING,
                                          pdTRUE,
                                          pdFALSE,
                                          portMAX_DELAY);
        
        //处理
        if(bits & ROUTINE_BIT_WIFI_SCANNING){

        }else{

        }
    }
    
}