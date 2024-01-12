#include "inputscene.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "lvgl.h"

#include "myWifi.h"
#include "status_routine.h"

#define TAG "InputS"

static void Keyboard_Wifi_Handler(lv_event_t* e);
static void Keyboard_CityInput_Handler(lv_event_t* e);
static bool CityInput_CheckLocation(const char* str);

lv_obj_t* inputScene;

extern char weatherUrl[130];//第一个location数字 index为96

void InputScene_Show(input_cb_type cb_type,void* p)
{   
    //这个函数将在各个回调函数中被调用，故不采用信号量

    inputScene=lv_obj_create(lv_scr_act());
    lv_obj_set_size(inputScene,240,240);
    lv_obj_set_style_bg_color(inputScene,lv_color_darken(lv_color_hex(0xffffff),LV_OPA_10),0);//颜色加深
    lv_obj_set_style_opa(inputScene,LV_OPA_100,LV_PART_MAIN);
    lv_obj_set_style_radius(inputScene,0,LV_PART_MAIN);
    
    lv_obj_clear_flag(inputScene, LV_OBJ_FLAG_SCROLLABLE);//滚动条居然占了键盘空间

    lv_obj_t* label_message=lv_label_create(inputScene);
    lv_obj_set_size(label_message,230,30);
    lv_obj_align(label_message,LV_ALIGN_TOP_LEFT,-10,0);

    lv_obj_t* textArea=lv_textarea_create(inputScene);
    lv_obj_align_to(textArea,label_message,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    lv_obj_set_width(textArea,240);
    lv_textarea_set_one_line(textArea, true);
    lv_textarea_set_max_length(textArea, 64);

    lv_obj_t* keyboard=lv_keyboard_create(inputScene);
    lv_obj_align(keyboard,LV_ALIGN_BOTTOM_MID,0,0);
    lv_obj_set_size(keyboard,240,120);
    lv_keyboard_set_textarea(keyboard,textArea);

    //指定不同的回调类型，从而用输入处理不同功能
    //加参数引入其他操作的对象
    switch (cb_type)
    {
    case INPUT_CB_WIFI_PWD:
        /* code */
        lv_label_set_text(label_message,"Input wifi password here");
        lv_obj_add_event_cb(keyboard,Keyboard_Wifi_Handler,LV_EVENT_ALL,p);
        break;
    case INPUT_CB_CITY:
        lv_label_set_text(label_message,"Input longitude and latitude\ne.g. 37.75:112.72");
        lv_obj_add_event_cb(keyboard,Keyboard_CityInput_Handler,LV_EVENT_ALL,NULL);
        break;
    default:
        lv_label_set_text(label_message,"no message");
        break;
    }
    
}


void Keyboard_Wifi_Handler(lv_event_t* e)
{
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* ssidLabel=(lv_obj_t* )lv_event_get_user_data(e);//用来访问myWifi中的ap_info

    lv_obj_t* keyboard=lv_event_get_target(e);

    lv_obj_t* wifi_widget=lv_obj_get_parent(keyboard);

    lv_obj_t* textArea=lv_obj_get_child(wifi_widget,1);
    if(code==LV_EVENT_READY){
        //Enter按下
        //开始联网
        MyWifi_Connect(false,lv_label_get_text(ssidLabel),lv_textarea_get_text(textArea));

        lv_obj_fade_out(wifi_widget,500,100);
        lv_obj_del(wifi_widget);
        inputScene=NULL;//防止被重新释放 
    }else if(code==LV_EVENT_CANCEL){
        //退出按下
        lv_obj_fade_out(wifi_widget,500,100);
        lv_obj_del(wifi_widget);
        inputScene=NULL;    
    }
}

void Keyboard_CityInput_Handler(lv_event_t* e){
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* keyboard=lv_event_get_target(e);
    
    lv_obj_t* widget=lv_obj_get_parent(keyboard);

    lv_obj_t* textArea=lv_obj_get_child(widget,1);
    if(code==LV_EVENT_READY){
        //Enter按下
        //检测输入正确性&修改经纬度
        const char* text=lv_textarea_get_text(textArea);
        if(CityInput_CheckLocation(text)){
            //修改weatherUrl
            xSemaphoreTake(semaphoreUrlChange,portMAX_DELAY);
            for (int i = 0; i <= strlen(text); i++)
            {
                if(i==strlen(text)) weatherUrl[i+96]='\0';
                else weatherUrl[i+96]=text[i];
            }
            xSemaphoreGive(semaphoreUrlChange);
            //把修改写入nvs
            nvs_handle_t nvs_handle;
            esp_err_t err=nvs_open("nvs",NVS_READWRITE,&nvs_handle);
            if (err!=ESP_OK)    ESP_LOGE(TAG,"nvs open fail %s",esp_err_to_name(err));
            else{
                err=nvs_set_str(nvs_handle,"location",text);
                err=nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
            }
            printf("New Url:%s\n",weatherUrl);         
            xEventGroupSetBits(eventGroup_note,ROUTINE_BIT_CITY_INPUT_SUCCESS);
        }else{
            xEventGroupSetBits(eventGroup_note,ROUTINE_BIT_CITY_INPUT_INVAILD);
        }
        lv_obj_fade_out(widget,500,100);
        lv_obj_del(widget);
        inputScene=NULL;
    }else if(code==LV_EVENT_CANCEL){
        //退出按下
        lv_obj_fade_out(widget,500,100);
        lv_obj_del(widget); 
        inputScene=NULL;   
    }
}

//不合法则返回0,其余情况返回结尾位置 
bool CityInput_CheckLocation(const char* str){
    int num1,num2,num3,num4;
    if(sscanf(str,"%d.%d:%d.%d",&num1,&num2,&num3,&num4)==4){
        if(0<=num1 && num1<=90){//纬度 在0-90度
            if(0<=num2 && num2<=99 && 0<=num4 && num4<=99){//小数范围有限
                if(0<=num3 && num3<=180){//经度 0-180 
                    return true;
                }
            }
        }        
    }
    return false;
}