#include "myWifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "lvgl.h"

#include "setupscene.h"
#include "status_routine.h"

#define TAG "my_wifi"

#define TEMP_SSID {'c','x','s','y','s'}
#define TEMP_PASS {'3','1','0','3','1','0','3','1','0'}

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define WIFI_MAX_RETRIES 5

//EventGroupHandle_t eventGroup_wifi;
extern SemaphoreHandle_t xGuiSemaphore;

static void Handler_WifiEvent(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
        
void Task_WifiInit(void* arg){
    //menuconfig中 wifi nvs flash使能时
    //主板上电/重新启动时，就不需从头开始配置 Wi-Fi 驱动程序，只需调用函数 esp_wifi_get_xxx API 获取之前存储的配置信息
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    //Wifi init
    //实现简单的连接
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg_wifiInit=WIFI_INIT_CONFIG_DEFAULT();
    
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifiInit));

    //向默认循环中注册事件
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        Handler_WifiEvent,
                                                        NULL,NULL
                                                        ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        Handler_WifiEvent,
                                                        NULL,NULL
                                                        ));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    //检查flash中的wifi_config信息
    wifi_config_t* p_cfg_wifi=NULL;
    if(esp_wifi_get_config(WIFI_IF_STA,p_cfg_wifi)==ESP_OK){
        //有信息则尝试连接
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,p_cfg_wifi));
        

        ESP_LOGI(TAG, "wifi_init_sta finished.");
    }else{
        //重新下载的话会抹除flash
        ESP_LOGI(TAG,"no flash,not doing anything");
    }

    ESP_ERROR_CHECK(esp_wifi_start());//WIFI_EVENT_STA_START 将随后产生
    

    //手动设置e.g.
    // wifi_config_t cfg_wifi={
    //     .sta={
    //         .ssid=TEMP_SSID,
    //         .password=TEMP_PASS,
    //         .threshold.authmode=WIFI_AUTH_WPA_WPA2_PSK,
    //         //.sae_h2e_identifier 与WAP3加密有关
    //     }
    // };


    //接收gui窗口发来的信号
    //eventGroup_wifi=xEventGroupCreate();

    // while(1){
    //     //阻塞，等待
    //     EventBits_t bits=xEventGroupWaitBits(eventGroup_lvgl2espWifi,LVGL2WIFI_CONNECT_BIT | LVGL2WIFI_SCAN_BIT,
    //                                         pdTRUE,pdFALSE,portMAX_DELAY);
        
    //     if(bits & LVGL2WIFI_SCAN_BIT){
    //         //gui发出扫描信号

    //     }else if(bits & LVGL2WIFI_CONNECT_BIT){
    //         //gui发出连接信号，连接特定ssid
            
    //     }
    // }
    
    vTaskDelete(NULL);//任务结束时删除是个好习惯
}

static void Handler_WifiEvent(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static uint8_t retryNum=0;

    if(event_base == WIFI_EVENT){//直接比较地址
        if(event_id==WIFI_EVENT_STA_START) esp_wifi_connect();
        else if(event_id==WIFI_EVENT_STA_DISCONNECTED){
            if(common_status.menu_wifi_switch==true){
                common_status.wifi=false;
                //只有开关未关时才重连
                if(retryNum<WIFI_MAX_RETRIES){
                    esp_wifi_connect();
                    retryNum++;
                    ESP_LOGI(TAG, "Retry to connect");
                }else{
                    ESP_LOGE(TAG, "Fail after n tries");
                    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
                    lv_label_set_text(common_status.label_wifiStatus,"No Wifi");
                    xSemaphoreGive(xGuiSemaphore);
                    xEventGroupSetBits(eventGroup_note,ROUTINE_BIT_WIFI_CONNECT_FAILED);
                }
            }

        }else if(event_id==WIFI_EVENT_STA_CONNECTED){
            wifi_event_sta_connected_t* event=(wifi_event_sta_connected_t*)event_data;
            xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
            lv_label_set_text_fmt(common_status.label_wifiStatus,"%s",event->ssid);
            xSemaphoreGive(xGuiSemaphore);           
        }
    }else if(event_base == IP_EVENT){
        if(event_id==IP_EVENT_STA_GOT_IP){
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Connect success");
            ESP_LOGI(TAG, "Hello?");
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            retryNum=0;
            
            common_status.wifi=true;
            common_status.menu_wifi_switch=true;

            xEventGroupSetBits(eventGroup_note,ROUTINE_BIT_WIFI_CONNECT_SUCCESS);
        }
    }
}

wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
void Task_WifiScan(void* args){
    xEventGroupSetBits(eventGroup_note,ROUTINE_BIT_WIFI_SCAN_START);

    lv_obj_t* subMenu=args;

    uint16_t number = SCAN_LIST_SIZE;
    
    uint16_t ap_count = 0;

    static lv_obj_t* wifiList[SCAN_LIST_SIZE];

    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
    for(int i=0;i<number;i++){
        if(lv_obj_is_valid(wifiList[i])) 
            lv_obj_del(wifiList[i]);
    }
    xSemaphoreGive(xGuiSemaphore);

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));//获取的所有记录
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));//
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
    if(lv_obj_is_valid(subMenu)){
        for(int i=0;i<number;i++){
            wifiList[i]=SubMenu_Stuff_Create(subMenu,
                                            LV_SYMBOL_WIFI,
                                            (char *)ap_info[i].ssid,
                                            SUBMENU_TYPE_LABEL,
                                            WifiMenu_Connect_Label_Handler,
                                            0,
                                            NULL);
            /*
            ps:参数的传递路线&方式
            将在整个widget被按下后调用WifiMenu_Connect_Label_Handler
            WifiMenu_Connect_Label_Handler获取widget中储存ssid的label并调用了inputscenc.c中的Inputscene_show 
            Inputscene_show把label传给Keyboard_Wifi_Handler
            并由Keyboard_Wifi_Handler从此处的label中取回ssid
            */
        }
    }

    //lv_obj_update_layout(subMenu);
    xSemaphoreGive(xGuiSemaphore);

    vTaskDelete(NULL);
}

esp_err_t MyWifi_Connect(char* ssid,char* pwd){
    //在inputscene里被键盘回调调用
    esp_wifi_stop();
    wifi_config_t cfg_wifi={
        .sta={
            .ssid="",
            .password="",
            .threshold.authmode=WIFI_AUTH_WPA_WPA2_PSK,
            //.sae_h2e_identifier 与WAP3加密有关
        }
    };
    strcpy((char*)cfg_wifi.sta.ssid,(char*)ssid);
    strcpy((char*)cfg_wifi.sta.password,(char*)pwd);
    
    ESP_LOGI(TAG,"ssid:%s",cfg_wifi.sta.ssid);
    ESP_LOGI(TAG,"password:%s",cfg_wifi.sta.password);
    esp_err_t e_type;
    if((e_type=esp_wifi_set_config(WIFI_IF_STA,&cfg_wifi))==ESP_OK){

    }else return e_type;

    if((e_type=esp_wifi_start())==ESP_OK){
        //start后将由esp_event_loop处理连接
    }else return e_type;

    return ESP_OK;
}