#include "input_device.h"

#include "driver/gpio.h"

#include "lvgl.h"

#include "status_routine.h"

lv_indev_t * indev_keypad;
lv_indev_t* indev_touchpad;

extern lv_obj_t* infoScene;
extern lv_obj_t* mainScene;
extern lv_obj_t* setupScene;

static void Touchpad_Init(void);

static void Touchpad_Read_Callback(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

static void InputFeedback_Callback(lv_indev_drv_t* indev_drv,uint8_t eventType);

void lv_port_indev_init(void){
    static lv_indev_drv_t indev_drv;//static防止可能lvgl库中还要使用
    
    // Keypad_Init();

    // lv_indev_drv_init(&indev_drv);
    // indev_drv.type=LV_INDEV_TYPE_ENCODER;
    // indev_drv.read_cb=Keypad_Read_Callback;

    // indev_keypad=lv_indev_drv_register(&indev_drv);

    Touchpad_Init();
    
    lv_indev_drv_init(&indev_drv);
    indev_drv.type=LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb=Touchpad_Read_Callback;
    indev_drv.feedback_cb=InputFeedback_Callback;
    indev_drv.long_press_time=800;
    indev_touchpad=lv_indev_drv_register(&indev_drv);
}


void Touchpad_Init(void){
    gpio_set_direction(GPIO_PIN_UP,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_UP,GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_PIN_MID,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_MID,GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_PIN_DOWN,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_DOWN,GPIO_PULLUP_ONLY);
    
    gpio_set_direction(GPIO_PIN_LEFT,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_LEFT,GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_PIN_RIGHT,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_RIGHT,GPIO_PULLUP_ONLY);

}

static uint8_t Touchpad_GetKey(){
    //为了检测多个按键，采用压位表示按下的按键
    uint8_t returnVal=0xff;
    //按下则置0
    if(gpio_get_level(GPIO_PIN_MID)==0)     returnVal &=~(1<<0);
    if(gpio_get_level(GPIO_PIN_UP)==0)      returnVal &=~(1<<1);
    if(gpio_get_level(GPIO_PIN_DOWN)==0)    returnVal &=~(1<<2);
    if(gpio_get_level(GPIO_PIN_LEFT)==0)    returnVal &=~(1<<3);
    if(gpio_get_level(GPIO_PIN_RIGHT)==0)   returnVal &=~(1<<4);
    return returnVal;
}

void Touchpad_Read_Callback(lv_indev_drv_t * indev_drv, lv_indev_data_t * data){
    //实现长时间按下

    //传参要求直接传入绝对坐标，而按键只能做相对位置更改，故储存绝对坐标
    static int32_t lastX=0,lastY=0;

    uint8_t returnVal=Touchpad_GetKey();

    if(~returnVal & 1){
        data->state=LV_INDEV_STATE_PRESSED;
        indev_drv->user_data=(void*)true;
    }
    else{
        data->state=LV_INDEV_STATE_RELEASED;
    }

    if(~returnVal & (1<<1)){//up
        if(lastY-POINTER_MOVE_SCALE>=0) lastY-=POINTER_MOVE_SCALE;
        indev_drv->user_data=false;
    }
          
    if(~returnVal & (1<<2)){//down
        if(lastY+POINTER_MOVE_SCALE<=lv_disp_get_ver_res(NULL)) lastY+=POINTER_MOVE_SCALE;
        indev_drv->user_data=false;
    }
        
    if(~returnVal & (1<<3))//left
        if(lastX-POINTER_MOVE_SCALE>=0) lastX-=POINTER_MOVE_SCALE;
    if(~returnVal & (1<<4))//right
        if(lastX+POINTER_MOVE_SCALE<=lv_disp_get_hor_res(NULL)) lastX+=POINTER_MOVE_SCALE;
    
    data->point.x=lastX;
    data->point.y=lastY;

}

extern lv_obj_t* currentSubMenu;
extern lv_obj_t* inputScene;

void InputFeedback_Callback(lv_indev_drv_t* indev_drv,uint8_t eventType){
    if(eventType==LV_EVENT_LONG_PRESSED && (int)indev_drv->user_data==true){
        //通过按键模拟触控板时，只有引起长按事件的按钮才会触发
        //keypad可以尝试在indev_drv的user data记录按键

        //实现在主页面长按跳转信息页面,以及反之
        if(lv_scr_act()==mainScene){
            lv_scr_load_anim(infoScene,LV_SCR_LOAD_ANIM_FADE_ON,500,100,false);
            lv_obj_set_parent(common_status.obj_statusBar,infoScene);
        }else{
            //在其他页面时长按直接跳转主界面
            if(currentSubMenu!=NULL) {
                lv_obj_del(currentSubMenu);
                currentSubMenu=NULL;
            }//防止内存泄漏！！！
            if(inputScene!=NULL){//可能没删除
                lv_obj_del(inputScene);
                inputScene=NULL;
            }
            lv_scr_load_anim(mainScene,LV_SCR_LOAD_ANIM_FADE_ON,500,100,false);
            lv_obj_set_parent(common_status.obj_statusBar,mainScene);
        }
    }
}

