#include "input_device.h"

#include "driver/gpio.h"

#include "lvgl.h"

lv_indev_t * indev_keypad;
lv_indev_t* indev_touchpad;

static void Touchpad_Init(void);

static void Touchpad_Read_Callback(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

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

    indev_touchpad=lv_indev_drv_register(&indev_drv);
}

//esp32输入初始化
// static void Keypad_Init(void){
//     gpio_set_direction(GPIO_PIN_UPPER,GPIO_MODE_INPUT);
//     gpio_set_pull_mode(GPIO_PIN_UPPER,GPIO_PULLUP_ONLY);

//     gpio_set_direction(GPIO_PIN_MID,GPIO_MODE_INPUT);
//     gpio_set_pull_mode(GPIO_PIN_MID,GPIO_PULLUP_ONLY);

//     gpio_set_direction(GPIO_PIN_LOWER,GPIO_MODE_INPUT);
//     gpio_set_pull_mode(GPIO_PIN_LOWER,GPIO_PULLUP_ONLY);
// }

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

// static uint32_t Keypad_GetKey(void){
//     //未按下返回0，其余返回上中下对应1、2、3
//     if(gpio_get_level(GPIO_PIN_UPPER)==0){
//         return 1;
//     }else if(gpio_get_level(GPIO_PIN_MID)==0){
//         return 2;
//     }else if(gpio_get_level(GPIO_PIN_LOWER)==0){
//         return 3;
//     }
//     return 0;
// }

//该函数会被Lvgl库定时调用
// static void Keypad_Read_Callback(lv_indev_drv_t * indev_drv, lv_indev_data_t * data){
//     static uint32_t last_key=0;

//     uint32_t act_key=Keypad_GetKey();

//     if(act_key != 0) {
//         data->state = LV_INDEV_STATE_PR;

//         /*Translate the keys to LVGL control characters according to your key definitions*/
//         switch(act_key) {
//         case 1:
//             act_key = LV_KEY_LEFT;//UPPER
//             break;
//         case 2:
//             act_key = LV_KEY_ENTER;//MID
//             break;
//         case 3:
//             act_key = LV_KEY_RIGHT;//LOWER
//             break;
//         }

//         last_key = act_key;
//     } else {
//         data->state = LV_INDEV_STATE_REL;
//     }

//     data->key = last_key;    
// }

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
    }
    else{
        data->state=LV_INDEV_STATE_RELEASED;
    }

    if(~returnVal & (1<<1))//up
        if(lastY-POINTER_MOVE_SCALE>=0) lastY-=POINTER_MOVE_SCALE;   
    if(~returnVal & (1<<2))//down
        if(lastY+POINTER_MOVE_SCALE<=lv_disp_get_ver_res(NULL)) lastY+=POINTER_MOVE_SCALE;
    if(~returnVal & (1<<3))//left
        if(lastX-POINTER_MOVE_SCALE>=0) lastX-=POINTER_MOVE_SCALE;
    if(~returnVal & (1<<4))//right
        if(lastX+POINTER_MOVE_SCALE<=lv_disp_get_hor_res(NULL)) lastX+=POINTER_MOVE_SCALE;
    
    data->point.x=lastX;
    data->point.y=lastY;

}

