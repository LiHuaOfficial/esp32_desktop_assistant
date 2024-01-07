#ifndef _MAIN_SCENE_
#define _MAIN_SCENE_

#ifdef __cplusplus
extern "C"{
#endif

#include "lvgl.h"

extern lv_obj_t* obj_weather;
extern lv_obj_t* obj_time;

void Task_MainScene(void * arg);

/// @brief 根据输入更改天气图片
/// @param code JSON文件中每种天气都有的id
void MainScene_FindImg(const char* code);

#ifdef __cplusplus
}
#endif

#endif