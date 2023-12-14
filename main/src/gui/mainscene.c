#include "mainscene.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lvgl.h"

#include "myWifi.h"
#include "status_routine.h"

extern SemaphoreHandle_t xGuiSemaphore;

extern lv_obj_t* mainScene;
extern lv_obj_t* setupScene;

extern lv_indev_t * indev_keypad;

lv_obj_t* obj_weather;
lv_obj_t* obj_time;
static void MenuEnter_Handler(lv_event_t* e);

//这个任务将在app_main的gui_task中被创建
//xPortGetCoreID();//拿这个函数获取当前任务运行的核心
//调用所有lv_...函数都应保证互斥访问
void Task_MainScene(void * arg){
    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);

    lv_obj_t* btn_setup = lv_btn_create(mainScene);
    //lv_obj_add_style(btn_setup,&style_menuObj,LV_PART_MAIN);
    lv_obj_set_align(btn_setup,LV_ALIGN_BOTTOM_LEFT);

    lv_obj_t* labelBtn=lv_label_create(btn_setup);
    lv_label_set_text(labelBtn,LV_SYMBOL_SETTINGS);

    lv_obj_add_event_cb(btn_setup,MenuEnter_Handler,LV_EVENT_CLICKED,NULL);

    //Wifi状态栏
    lv_obj_t* obj_statusBar=lv_obj_create(mainScene);
    lv_obj_set_align(obj_statusBar,LV_ALIGN_TOP_LEFT);

    lv_obj_set_style_border_width(obj_statusBar,0,LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj_statusBar,0,0);
    lv_obj_set_style_radius(obj_statusBar, 0, 0);
    lv_obj_set_size(obj_statusBar,lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL)/8);
    lv_obj_clear_flag(obj_statusBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj_statusBar,-10,-10);

    lv_obj_t* label_wifiLogo=lv_label_create(obj_statusBar);
    lv_label_set_text(label_wifiLogo,LV_SYMBOL_WIFI);
    lv_obj_set_align(label_wifiLogo,LV_ALIGN_TOP_LEFT);

    common_status.label_wifiStatus=lv_label_create(obj_statusBar);
    lv_label_set_text(common_status.label_wifiStatus,"No Wifi");
    lv_obj_align_to(common_status.label_wifiStatus,label_wifiLogo,LV_ALIGN_LEFT_MID,17,0);

    //时间框
    obj_time=lv_obj_create(mainScene);
    lv_obj_clear_flag(obj_time,LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_pos(obj_time,0,20);
    lv_obj_align(obj_time,LV_ALIGN_TOP_MID,0,20);
    lv_obj_set_size(obj_time,230,90);
    lv_obj_t* label_time=lv_label_create(obj_time);
    lv_obj_align(label_time,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_text_font(label_time,&lv_font_montserrat_48,0);

    lv_obj_t* label_date=lv_label_create(obj_time);
    lv_obj_align_to(label_date,label_time,LV_ALIGN_OUT_BOTTOM_MID,-20,0);
    //天气框
    obj_weather=lv_obj_create(mainScene);
    lv_obj_clear_flag(obj_weather,LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_size(obj_weather,200,80);
    lv_obj_align_to(obj_weather,obj_time,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    lv_obj_t* label_place=lv_label_create(obj_weather);
    lv_obj_t* label_weather=lv_label_create(obj_weather);
    lv_obj_align_to(label_weather,label_place,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_t* label_temperature=lv_label_create(obj_weather);
    lv_obj_align_to(label_temperature,label_weather,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    lv_label_set_long_mode(label_place,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(label_weather,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(label_temperature,LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_label_set_text(label_place,      "City:   None");
    lv_label_set_text(label_weather,    "Weather:None");
    lv_label_set_text(label_temperature,"Temp:   None");

    lv_obj_move_foreground(btn_setup);
    xSemaphoreGive(xGuiSemaphore);

    vTaskDelete(NULL);
}


void MenuEnter_Handler(lv_event_t *e)
{
    //xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
    lv_scr_load_anim(setupScene,LV_SCR_LOAD_ANIM_MOVE_RIGHT,200,50,false);
    //lv_indev_set_group(indev_keypad,group_setupScene_default);
    //xSemaphoreGive(xGuiSemaphore);
}
