#ifndef _INPUT_SCENE_
#define _INPUT_SCENE_

#ifdef __cplusplus
extern "C"{
#endif

/*
当需要输入时调用该函数显示

点击确定时回调函数获取文本并执行相应功能
（和其他对象互动通过全局变量传给回调函数参数）
*/

typedef enum{
    INPUT_CB_WIFI_PWD,
    INPUT_CB_NONE
}input_cb_type;

/// @brief 显示键盘，文本框，提供获取输入的回调
/// @param cb_type 指定回调函数
void InputScene_Show(input_cb_type cb_type,void* p);

#ifdef __cplusplus
}
#endif

#endif