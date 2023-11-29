#ifndef _HTTP_
#define _HTTP_

#ifdef __cplusplus
extern "C"{
#endif

#define HTTP_REQUST_INTERVAL_MS (1000*20)
#define HTTP_OUT_PUT_BUFFER_SIZE 2048

void Task_Http(void* arg);

#ifdef __cplusplus
}
#endif

#endif