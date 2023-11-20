#include "inputscene.h"

#include "esp_wifi.h"

#include "lvgl.h"

#include "myWifi.h"

static void Keyboard_Wifi_Handler(lv_event_t* e);

void InputScene_Show(input_cb_type cb_type,void* p)
{   
    //这个函数将在各个回调函数中被调用，故不采用信号量

    lv_obj_t* inputScene=lv_obj_create(lv_scr_act());
    lv_obj_set_size(inputScene,lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_color(inputScene,lv_color_darken(lv_color_hex(0xffffff),LV_OPA_10),0);//颜色加深
    lv_obj_set_style_opa(inputScene,LV_OPA_80,LV_PART_MAIN);

    lv_obj_clear_flag(inputScene, LV_OBJ_FLAG_SCROLLABLE);//滚动条居然占了键盘空间

    lv_obj_t* textArea=lv_textarea_create(inputScene);
    lv_textarea_set_one_line(textArea, true);
    lv_textarea_set_max_length(textArea, 64);

    lv_obj_t* keyboard=lv_keyboard_create(inputScene);
    lv_keyboard_set_textarea(keyboard,textArea);

    //指定不同的回调类型，从而用输入处理不同功能
    //加参数引入其他操作的对象
    switch (cb_type)
    {
    case INPUT_CB_WIFI_PWD:
        /* code */
        lv_obj_add_event_cb(keyboard,Keyboard_Wifi_Handler,LV_EVENT_ALL,p);
        break;
    
    default:
        break;
    }
    
}


void Keyboard_Wifi_Handler(lv_event_t* e)
{
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* ssidLabel=(lv_obj_t* )lv_event_get_user_data(e);//用来访问myWifi中的ap_info

    lv_obj_t* keyboard=lv_event_get_target(e);

    lv_obj_t* wifi_widget=lv_obj_get_parent(keyboard);

    lv_obj_t* textArea=lv_obj_get_child(wifi_widget,0);
    if(code==LV_EVENT_READY){
        //Enter按下
        //开始联网
        //可能还要准备一个专用于通知的任务
        MyWifi_Connect(lv_label_get_text(ssidLabel),lv_textarea_get_text(textArea));

        lv_obj_fade_out(wifi_widget,500,100);
        lv_obj_del(wifi_widget); 
    }else if(code==LV_EVENT_CANCEL){
        //退出按下
        lv_obj_fade_out(wifi_widget,500,100);
        lv_obj_del(wifi_widget);    
    }   
}
