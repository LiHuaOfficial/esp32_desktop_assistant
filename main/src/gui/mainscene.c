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
    common_status.obj_statusBar=lv_obj_create(mainScene);
    lv_obj_set_align(common_status.obj_statusBar,LV_ALIGN_TOP_LEFT);

    lv_obj_set_style_border_width(common_status.obj_statusBar,0,LV_PART_MAIN);
    lv_obj_set_style_outline_width(common_status.obj_statusBar,0,0);
    lv_obj_set_style_radius(common_status.obj_statusBar, 0, 0);
    lv_obj_set_size(common_status.obj_statusBar,lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL)/8);
    lv_obj_clear_flag(common_status.obj_statusBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(common_status.obj_statusBar,-10,-10);

    lv_obj_t* label_wifiLogo=lv_label_create(common_status.obj_statusBar);
    lv_label_set_text(label_wifiLogo,LV_SYMBOL_WIFI);
    lv_obj_set_align(label_wifiLogo,LV_ALIGN_TOP_LEFT);

    lv_obj_t* label_wifiStatus=lv_label_create(common_status.obj_statusBar);
    lv_label_set_text(label_wifiStatus,"No Wifi");
    lv_label_set_long_mode(label_wifiStatus,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_wifiStatus,40);
    lv_obj_align_to(label_wifiStatus,label_wifiLogo,LV_ALIGN_OUT_RIGHT_MID,0,0);

    lv_obj_t* label_placeLogo=lv_label_create(common_status.obj_statusBar);
    lv_label_set_text(label_placeLogo,LV_SYMBOL_GPS);
    lv_obj_align_to(label_placeLogo,label_wifiStatus,LV_ALIGN_OUT_RIGHT_MID,20,0);

    lv_obj_t* label_placeStatus=lv_label_create(common_status.obj_statusBar);
    lv_label_set_text(label_placeStatus,"None");
    lv_label_set_long_mode(label_placeStatus,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_placeStatus,40);
    lv_obj_align_to(label_placeStatus,label_placeLogo,LV_ALIGN_OUT_RIGHT_MID,0,0);
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

    lv_obj_t* label_weather=lv_label_create(obj_weather);
    lv_obj_align(label_weather,LV_ALIGN_TOP_LEFT,0,0);
    lv_obj_t* label_temperature=lv_label_create(obj_weather);
    lv_obj_align_to(label_temperature,label_weather,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    lv_obj_t* img_weatherLogo=lv_img_create(obj_weather);
    lv_obj_align(img_weatherLogo,LV_ALIGN_TOP_RIGHT,0,0);

    lv_label_set_long_mode(label_weather,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(label_temperature,LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_label_set_text(label_weather,    "None");
    lv_label_set_text(label_temperature,"None");

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

//https://docs.seniverse.com/api/start/code.html
//根据网址记录JSON中id到图片的映射
const uint8_t findImg[]={0,1,0,1,               //晴
                         2,2,2,2,2,2,           //多云
                         4,4,4,4,4,4,4,4,4,4,4, //雨
                         6,6,6,6,6,             //雪
                         5,5,5,5,               //沙尘
                         3,3,                   //雾
                         7,7,7,7,7,              //风
                         6,0};                   //冷&热

//记录文件名字
const char imgPath[][16]={
    "sunny.bin",        //0
    "sunny_night.bin",  //1
    "cloudy.bin",       //2
    "foggy.bin",        //3
    "rain.bin",         //4
    "sandy.bin",        //5
    "snow.bin",         //6
    "windy.bin"         //7
};

void MainScene_FindImg(const char* code){
    uint8_t id=0,i=0;
    while ('0'<=code[i] && code[i]<='9')
    {
        id*=10;
        id+=(code[i]-'0');
        i++;
    }

    //S:/spiffs/ 10字符 
    char imgSrc[26]="S:/spiffs/"; 
    if(id>38) return;
    else {
        //手动连接字符串
        for(uint8_t i=10;i-10<strlen(imgPath[findImg[id]]);i++){
            imgSrc[i]=imgPath[findImg[id]][i-10];
        }
        lv_img_set_src(lv_obj_get_child(obj_weather,2),imgSrc);
    }
        
}