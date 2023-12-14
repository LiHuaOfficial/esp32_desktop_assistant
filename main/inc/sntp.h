#ifndef _SNTP_
#define _SNTP_

#ifdef __cplusplus
extern "C"{
#endif

#include<sys/time.h>
#include<time.h>

void SNTP_init();
void SNTP_Update();
struct tm SNTP_GetTime();

#ifdef __cplusplus
}
#endif

#endif