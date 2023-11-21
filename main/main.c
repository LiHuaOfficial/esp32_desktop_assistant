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
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

//#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"
//wifi


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

// lv_group_t* group_mainScene;
// lv_group_t* group_setupScene_default;

TaskHandle_t TaskHandle_MainScene;
TaskHandle_t TaskHandle_SetupScene;
TaskHandle_t TaskHandle_Wifi;

SemaphoreHandle_t xGuiSemaphore;


static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);

static void Task_LED(void *arg);



/**********************
 *   APPLICATION MAIN
 **********************/
void app_main()
{
    //LED init
    gpio_set_direction(GPIO_NUM_32,GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32,1);

    xTaskCreate(Task_LED,"LED",4096,NULL,0,NULL);
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL,GUI_PRIORITY, NULL, 1);

    
    //xTaskCreate(Task_ADC,"ADC",4096,NULL,0,NULL);
    
}
 
/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */


static void guiTask(void *pvParameter)
{
 
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
 
    lv_init();
 
    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();
    
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
 
    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif
 
    static lv_disp_draw_buf_t disp_buf;
 
    uint32_t size_in_px = DISP_BUF_SIZE;
 
#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820 || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306
    size_in_px *= 8;
#endif
 
    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);
 
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
 
    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif
 
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    lv_disp_drv_register(&disp_drv);
 
    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif
 
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
    //设置默认输入组
    // lv_group_t *g=lv_group_create();
    // lv_group_set_default(g);
    // lv_indev_set_group(indev_keypad,g);
    /* Create the demo application */
    //create_demo_application();

    //实例化屏幕
    mainScene=lv_scr_act();
    setupScene=lv_obj_create(NULL);

    xTaskCreatePinnedToCore(Task_MainScene,"MainScene",4096*2,NULL,GUI_PRIORITY,&TaskHandle_MainScene,1);
    xTaskCreatePinnedToCore(Task_SetupScene,"SetupScene",4096*3,NULL,GUI_PRIORITY,&TaskHandle_SetupScene,1);

    xTaskCreate(Task_Routine,"Routine",4096,NULL,configMAX_PRIORITIES,NULL);

    xTaskCreate(Task_WifiInit,"Wifi",4096*5,NULL,WIFI_PRIORTY,&TaskHandle_Wifi);
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
 
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
 
    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
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




