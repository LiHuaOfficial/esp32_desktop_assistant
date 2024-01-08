#ifndef _SETUP_SCENE_
#define _SETUP_SCENE_

#ifdef __cplusplus
extern "C"{
#endif

#include "lvgl.h"

typedef enum 
{
    SUBMENU_TYPE_CHECKBOX,
    SUBMENU_TYPE_SLIDER,
    SUBMENU_TYPE_SWITCH,
    SUBMENU_TYPE_LABEL,
    SUBMENU_TYPE_BUTTON,
    SUBMENU_TYPE_NOTHING
} submenu_type_t;

void SetupScene_Create(void);

/// @brief 生成菜单栏元素
/// @param parent 指定其父页面
/// @param text 写在左边的说明
/// @param objText 指定给子对象的文字
/// @param type 子对象类型
/// @param stuff_event_cb 子对象回调函数
/// @param value 给子对象的参数
/// @param args 给回调函数的参数
/// @return 菜单栏对象
lv_obj_t* SubMenu_Stuff_Create(lv_obj_t* parent,const char* text,const char* objText,
                                submenu_type_t type, lv_event_cb_t stuff_event_cb, uint32_t value,
                                void* args);


/// @brief 添加Wifi项时每一项的回调函数
/// @param e 
void WifiMenu_Connect_Label_Handler(lv_event_t* e);
#ifdef __cplusplus
}
#endif

#endif