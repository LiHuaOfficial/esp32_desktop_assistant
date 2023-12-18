//////////////////////注意事项/////////////////////////
/*
背光控制 components\lvgl_esp32_drivers\lvgl_tft\esp_lcd_backlight.c
全部注释掉了，置高电平即可
*/
//////////////////////////////////////////////////////


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

//#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#include "demos/lv_demos.h"
#else
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#endif
 
 
#include "lvgl_helpers.h"
 
#include "input_device.h"
#include "mainscene.h"
#include "setupscene.h"
#include "myWifi.h"
#include "status_routine.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

#define PINX_ADC_CHANNEL ADC2_CHANNEL_3
#define PINY_ADC_CHANNEL ADC2_CHANNEL_0

#define GUI_PRIORITY 6
#define WIFI_PRIORTY 5
/**********************
 *  STATIC PROTOTYPES
 **********************/
extern lv_indev_t* indev_keypad;
extern lv_indev_t* indev_touchpad;

lv_obj_t* mainScene;
lv_obj_t* setupScene;
lv_obj_t* infoScene;

lv_fs_drv_t drv;
// lv_group_t* group_mainScene;
// lv_group_t* group_setupScene_default;

TaskHandle_t TaskHandle_MainScene;
TaskHandle_t TaskHandle_SetupScene;
TaskHandle_t TaskHandle_Wifi;

SemaphoreHandle_t xGuiSemaphore;


static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);

static void Task_LED(void *arg);

void* my_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
lv_fs_res_t my_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
lv_fs_res_t my_close_cb(lv_fs_drv_t * drv, void * file_p);
lv_fs_res_t my_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t my_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);

/**********************
 *   APPLICATION MAIN
 **********************/
void app_main()
{   
    //spiffs init
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 7,
      .format_if_mount_failed = false
    };
    // Use settings defined above to initialize and mount SPIFFS filesystem. mount v.挂载
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) ESP_LOGE(TAG, "Failed to mount or format filesystem");
        else if (ret == ESP_ERR_NOT_FOUND) ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        else ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    else ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);

    //LED init
    gpio_set_direction(GPIO_NUM_32,GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32,1);

    xTaskCreate(Task_LED,"LED",4096,NULL,0,NULL);
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL,GUI_PRIORITY, NULL, 1);
}

static void guiTask(void *pvParameter)
{
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    lvgl_driver_init();//from lvgl_esp32_driver
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    /* Use double buffered when not working with monochrome displays */
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
    static lv_disp_draw_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    lv_disp_drv_register(&disp_drv);
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui",
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));//参数单位应该是微秒
    
    //初始化输入设备
    lv_port_indev_init();
    lv_obj_t* cursor=lv_img_create(lv_scr_act());
    lv_img_set_src(cursor,LV_SYMBOL_CLOSE);
    lv_indev_set_cursor(indev_touchpad,cursor);

    //初始化Lvgl文件系统                   
    lv_fs_drv_init(&drv);                     /*Basic initialization*/
    drv.letter = 'S';                         /*An uppercase letter to identify the drive */
    drv.open_cb = my_open_cb;                 /*Callback to open a file */
    drv.close_cb = my_close_cb;               /*Callback to close a file */
    drv.read_cb = my_read_cb;                 /*Callback to read a file */
    drv.seek_cb = my_seek_cb;                 /*Callback to seek in a file (Move cursor) */
    drv.tell_cb = my_tell_cb;  
    lv_fs_drv_register(&drv);                 /*Finally register the drive*/

    //实例化屏幕
    mainScene=lv_scr_act();
    setupScene=lv_obj_create(NULL);
    infoScene=lv_obj_create(NULL);
    xTaskCreatePinnedToCore(Task_MainScene,"MainScene",4096*2,NULL,GUI_PRIORITY,&TaskHandle_MainScene,1);
    xTaskCreatePinnedToCore(Task_SetupScene,"SetupScene",4096*2,NULL,GUI_PRIORITY,&TaskHandle_SetupScene,1);

    xTaskCreate(Task_Routine,"Routine",4096,NULL,configMAX_PRIORITIES,NULL);

    xTaskCreate(Task_WifiInit,"Wifi",4096*2,NULL,WIFI_PRIORTY,&TaskHandle_Wifi);
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
 
        /* Try to take the semaphore, call lvgl related function on success */
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);    
        lv_task_handler();
        xSemaphoreGive(xGuiSemaphore);
    }
}
 
static void lv_tick_task(void *arg)
{
    (void)arg;
    //printf("hello\n");
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void Task_LED(void *arg){
    uint8_t ledState=0;
    gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
    while (1)
    {
        gpio_set_level(GPIO_NUM_2,ledState);
        ledState=!ledState;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    } 
}

lv_fs_res_t my_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    *br=fread(buf,1,btr,file_p);//byte by byte
    return LV_FS_RES_OK;
}

void* my_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    if(mode==LV_FS_MODE_RD){
        FILE* f=fopen(path,"r");
        return f;
    }else return NULL;
}

lv_fs_res_t my_close_cb(lv_fs_drv_t * drv, void * file_p){
    if(!fclose(file_p)) return LV_FS_RES_OK;
    else return LV_FS_RES_DENIED;
}

lv_fs_res_t my_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence){
    int ret=1;
    switch (whence)
    {
    case LV_FS_SEEK_SET:
        ret=fseek(file_p,pos,SEEK_SET);
        break;
    case LV_FS_SEEK_CUR:
        ret=fseek(file_p,pos,SEEK_CUR);
        break;
    case LV_FS_SEEK_END:
        ret=fseek(file_p,pos,SEEK_END);
        break;
    }
    if(!ret) return LV_FS_RES_OK;
    else return LV_FS_RES_DENIED;
}
lv_fs_res_t my_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p){
    *pos_p=ftell(file_p);
    if((*pos_p)==(uint32_t)(-1)) return LV_FS_RES_DENIED;
    else return LV_FS_RES_OK;
}