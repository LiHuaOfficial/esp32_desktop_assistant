#ifndef _INPUT_DEVICE_
#define _INPUT_DEVICE_

#ifdef __cplusplus
extern "C"{
#endif

#define GPIO_PIN_UP 15
#define GPIO_PIN_DOWN 4
#define GPIO_PIN_LEFT 19
#define GPIO_PIN_RIGHT 21
#define GPIO_PIN_MID 22

#define POINTER_MOVE_SCALE 5
//在lvgl初始化后调用以注册输入设备
void lv_port_indev_init(void);

#ifdef __cplusplus
}
#endif

#endif