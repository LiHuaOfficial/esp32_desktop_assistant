#include "http.h"

#include "string.h"
#include "stdlib.h"

#include "status_routine.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_system.h"

#define TAG "http"
#define MIN(X,Y) (((X)>(Y))?(X):(Y))
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
        .url="http://httpbin.org/get",
        .event_handler=Event_Handler_Http,
        .user_data=response_buffer,
        .disable_auto_redirect=true,
    };
    esp_http_client_handle_t handle_http_client=esp_http_client_init(&cfg_http_client);

    if(esp_http_client_perform(handle_http_client)==ESP_OK){
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(handle_http_client),
                esp_http_client_get_content_length(handle_http_client));        
    }else{
        ESP_LOGE(TAG,"Http GET request failed");
    }
    //ESP_LOG_BUFFER_HEX(TAG,response_buffer,strlen(response_buffer));
    printf("\n%s\n",response_buffer);
    esp_http_client_cleanup(handle_http_client);
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
