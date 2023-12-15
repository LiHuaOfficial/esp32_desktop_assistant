#ifndef _MAIN_SCENE_
#define _MAIN_SCENE_

#ifdef __cplusplus
extern "C"{
#endif

#include "lvgl.h"

extern lv_obj_t* obj_weather;
extern lv_obj_t* obj_time;

void Task_MainScene(void * arg);

void MainScene_FindImg(const char* code);
#ifdef __cplusplus
}
#endif

#endif