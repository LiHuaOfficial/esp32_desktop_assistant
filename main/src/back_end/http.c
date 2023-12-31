#include "http.h"

#include "status_routine.h"

#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_system.h"

#include "cJSON.h"
#include "mainscene.h"

#define TAG "http"
#define MIN(X,Y) (((X)>(Y))?(X):(Y))

extern SemaphoreHandle_t xGuiSemaphore;

static void Http_get_from_url();

void Task_Http(void* arg)
{   
    while (1)
    {
        vTaskDelay(HTTP_REQUST_INTERVAL_MS/portTICK_PERIOD_MS);
        if(common_status.wifi==true){
            Http_get_from_url();
        }
    }
}

void Http_get_from_url()
{
    char response_buffer[HTTP_OUT_PUT_BUFFER_SIZE];
    /*流方法CPU占用低了5%左右,且不会在get_status_code时莫名其妙崩溃！！！*/
    esp_http_client_config_t cfg_http_client={
        .url="https://api.seniverse.com/v3/weather/now.json?key=SJvAOXfRZ1pDGsd3D&location=37.75:112.72&language=en&unit=c",
        //.event_handler=Event_Handler_Http,
        //.user_data=response_buffer,
        //.disable_auto_redirect=true,
    };
    esp_http_client_handle_t handle_http_client=esp_http_client_init(&cfg_http_client);
    esp_http_client_set_method(handle_http_client,HTTP_METHOD_GET);
    
    int64_t contentLenth=0;
    
    if(ESP_OK!=esp_http_client_open(handle_http_client,0)){
        ESP_LOGE(TAG,"http_client open failed");
    }else{
        contentLenth=esp_http_client_fetch_headers(handle_http_client);

        if(contentLenth<0){
            ESP_LOGE(TAG,"http_client fetch header failed");
        }else{
            contentLenth=esp_http_client_read_response(handle_http_client,response_buffer,HTTP_OUT_PUT_BUFFER_SIZE);
            if(contentLenth>=0){
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(handle_http_client),				//获取响应状态信息
                esp_http_client_get_content_length(handle_http_client));
                for (int i = 0; i < contentLenth; i++)
                {
                    printf("%c",response_buffer[i]);
                }
                printf("\n");
            }else{
                ESP_LOGW(TAG,"http_client read response failed");
            }
        }
    }
    esp_http_client_close(handle_http_client);
    esp_http_client_cleanup(handle_http_client);
    //解析Json文件
    if(contentLenth>0){
        cJSON* resJSON=cJSON_ParseWithLength(response_buffer,contentLenth);
        
        cJSON* json_array=cJSON_GetObjectItem(resJSON,"results");
        cJSON* json_arrayItem=cJSON_GetArrayItem(json_array,0);
        cJSON* json_obj_location=cJSON_GetObjectItem(json_arrayItem,"location");
        cJSON* json_obj_now=cJSON_GetObjectItem(json_arrayItem,"now");

        //从Location中取城市名
        cJSON* json_obj_name=cJSON_GetObjectItem(json_obj_location,"name");

        //从now中取得天气&温度
        cJSON* json_obj_text=cJSON_GetObjectItem(json_obj_now,"text");
        cJSON* json_obj_temperature=cJSON_GetObjectItem(json_obj_now,"temperature");
        cJSON* json_obj_code=cJSON_GetObjectItem(json_obj_now,"code");
        xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
        lv_label_set_text_fmt(lv_obj_get_child(common_status.obj_statusBar,3),"%s",json_obj_name->valuestring);
        lv_label_set_text_fmt(lv_obj_get_child(obj_weather,0),"%s",json_obj_text->valuestring);
        lv_label_set_text_fmt(lv_obj_get_child(obj_weather,1),"%s°C",json_obj_temperature->valuestring);
        MainScene_FindImg(json_obj_code->valuestring);
        xSemaphoreGive(xGuiSemaphore);

        cJSON_Delete(resJSON);
        
    }
}
