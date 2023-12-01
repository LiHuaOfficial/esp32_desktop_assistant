#include "http.h"

#include "string.h"
#include "stdlib.h"

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
static esp_err_t Event_Handler_Http(esp_http_client_event_t *evt);

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
    
    esp_http_client_config_t cfg_http_client={
        .url="https://api.seniverse.com/v3/weather/now.json?key=SJvAOXfRZ1pDGsd3D&location=37.75:112.72&language=en&unit=c",
        .event_handler=Event_Handler_Http,
        .user_data=response_buffer,
        .disable_auto_redirect=true,
    };
    esp_http_client_handle_t handle_http_client=esp_http_client_init(&cfg_http_client);
    
    int64_t contentLenth;
    if(esp_http_client_perform(handle_http_client)==ESP_OK){
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(handle_http_client),
                contentLenth=esp_http_client_get_content_length(handle_http_client));        
    }else{
        ESP_LOGE(TAG,"Http GET request failed");
        contentLenth=0;
    }
    //ESP_LOG_BUFFER_HEX(TAG,response_buffer,strlen(response_buffer));
    for(int64_t i=0;i<contentLenth;i++){
        printf("%c",response_buffer[i]);
    }
    esp_http_client_cleanup(handle_http_client);

    //解析Json文件
    if(contentLenth){
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

        xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
        lv_label_set_text_fmt(lv_obj_get_child(obj_weather,0),"City:   %s",json_obj_name->valuestring);
        lv_label_set_text_fmt(lv_obj_get_child(obj_weather,1),"Weather:%s",json_obj_text->valuestring);
        lv_label_set_text_fmt(lv_obj_get_child(obj_weather,2),"Temp:   %s°C",json_obj_temperature->valuestring);
        xSemaphoreGive(xGuiSemaphore);

        cJSON_Delete(resJSON);
        
    }
}

esp_err_t Event_Handler_Http(esp_http_client_event_t* e)
{
    static char *output_buffer=NULL;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(e->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", e->header_key, e->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", e->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             *  分块编码没法解析
             */
            if (!esp_http_client_is_chunked_response(e->client)) {
                int copy_len = 0;
                if (e->user_data) {//在参数中指定了缓存区
                    copy_len = MIN(e->data_len, (HTTP_OUT_PUT_BUFFER_SIZE - output_len));
                    if (copy_len) {
                        memcpy(e->user_data + output_len, e->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(e->client, "From", "user@example.com");
            esp_http_client_set_header(e->client, "Accept", "text/html");
            esp_http_client_set_redirection(e->client);
            break;
    }
    return ESP_OK;    
}
