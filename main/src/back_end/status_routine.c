#include "status_routine.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "http.h"

#define TAG "NOTE"

extern SemaphoreHandle_t xGuiSemaphore;

//每当需要产生可见通知时发送bit
EventGroupHandle_t eventGroup_note;

Common_status common_status={
    .wifi=false,
    .label_wifiStatus=NULL,
    .menu_wifi_switch=false
};

static void Generate_NoteWidget(char* noteText);

void Task_MyEventHandle(void* arg);

void Task_Routine(void *arg)
{
    xTaskCreate(Task_MyEventHandle,"MyEvent",4096,NULL,configMAX_PRIORITIES,NULL);

    //需要定时处理的操作
    xTaskCreate(Task_Http,"Http",4096*3,NULL,5,NULL);

    vTaskDelete(NULL);
}

void Task_MyEventHandle(void* arg){
    //处理事件
    eventGroup_note=xEventGroupCreate();

    while (1)
    {
        //等待
        uint32_t bits=xEventGroupWaitBits(eventGroup_note,
                                          ROUTINE_BIT_WIFI_SCAN_START | ROUTINE_BIT_WIFI_CONNECT_SUCCESS | ROUTINE_BIT_WIFI_CONNECT_FAILED,
                                          pdTRUE,
                                          pdFALSE,
                                          portMAX_DELAY);
        
        //处理
        if(bits & ROUTINE_BIT_WIFI_SCAN_START){
            ESP_LOGI(TAG,"Scanning");
            Generate_NoteWidget("Scanning");
        }else if(bits & ROUTINE_BIT_WIFI_CONNECT_SUCCESS){
            ESP_LOGI(TAG,"Wifi Connect Success");
            Generate_NoteWidget("Wifi Connect Success");
        }else if(bits & ROUTINE_BIT_WIFI_CONNECT_FAILED){
            ESP_LOGI(TAG,"Wifi Connect Failed");
            Generate_NoteWidget("Wifi Connect Failed");
        }
    }
}

static void DelTimer_Cb(lv_timer_t* timer);
//提示状态的小弹窗
static void Generate_NoteWidget(char* noteText){
    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
    lv_obj_t* widget_note=lv_obj_create(lv_scr_act());
    lv_obj_set_height(widget_note,45);
    lv_obj_align_to(widget_note,NULL,LV_ALIGN_BOTTOM_MID,0,-60);
    lv_obj_clear_flag(widget_note,LV_OBJ_FLAG_SCROLLABLE);
    

    lv_obj_t* label_note=lv_label_create(widget_note);
    lv_label_set_text(label_note,noteText);
    lv_obj_set_size(label_note,lv_obj_get_width(widget_note),lv_obj_get_height(widget_note));
    lv_label_set_long_mode(label_note,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_move_foreground(widget_note);
    // lv_timer_t* timer_delNote=lv_timer_create(DelTimer_Cb,2000,widget_note);
    // lv_timer_set_repeat_count(timer_delNote,1);
    //lv_timer_ready(timer_delNote);
    lv_obj_fade_out(widget_note,500,2000);
    xSemaphoreGive(xGuiSemaphore);

} 

void DelTimer_Cb(lv_timer_t *timer)
{
    lv_obj_t* widget_note=timer->user_data;
    lv_obj_del(widget_note);
}