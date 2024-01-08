#include "sht3x.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "SHT3x"

uint8_t resultBuffer[6];

static esp_err_t I2C_Init(void){
    i2c_config_t i2c_cfg={
        .mode=I2C_MODE_MASTER,
        .sda_io_num=I2C_SDA_GPIO,
        .sda_pullup_en=GPIO_PULLUP_ENABLE,
        .scl_io_num=I2C_SCL_GPIO,
        .scl_pullup_en=GPIO_PULLUP_ENABLE,
        .master.clk_speed=I2C_MASTER_FREQ,
        .clk_flags=0
    };

    i2c_param_config(I2C_NUM_0,&i2c_cfg);

    return i2c_driver_install(I2C_NUM_0,i2c_cfg.mode,0,0,0);

    
}

//发送两字节指令（所有的指令都是两字节）
static void SHT3x_WriteCommand(uint16_t data,bool needStop){
    i2c_cmd_handle_t cmd=i2c_cmd_link_create();
    esp_err_t err;
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,SHT30_ADDR<<1 | I2C_WR,true);
    i2c_master_write_byte(cmd,(data>>8) & 0xff,      true);
    i2c_master_write_byte(cmd,data & 0xff,           true);

    if(needStop){
        i2c_master_stop(cmd);
    }

    err=i2c_master_cmd_begin(I2C_NUM_0,cmd,100);
    if(err!=ESP_OK) ESP_LOGE(TAG,"in write cmd:%s current cmd:%x",esp_err_to_name(err),data); 
    i2c_cmd_link_delete(cmd);
} 

static uint8_t CRC_Check(uint8_t *check_data,uint8_t num,uint8_t check_crc)
{
    uint8_t bit;        // bit mask
    uint8_t crc = 0xFF; // calculated checksum
    uint8_t byteCtr;    // byte counter
    
 // calculates 8-Bit checksum with given polynomial x8+x5+x4+1
    for(byteCtr = 0; byteCtr < num; byteCtr++)
    {
        crc ^= (*(check_data+byteCtr));
        //crc校验，最高位是1就^0x31
        for(bit = 8; bit > 0; --bit)
        {
            if(crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
    if(crc==check_crc)
        return 1;
    else 
        return 0;
}

//只有两种数据返回，温湿度和寄存器
static bool SHT3x_ReadTempAndHumid(float* temp,float* humi){
    SHT3x_WriteCommand(0xe000,false);
    vTaskDelay(5);

    esp_err_t err;
    i2c_cmd_handle_t cmd=i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,SHT30_ADDR<<1 | I2C_RD,true);
    i2c_master_read(cmd,resultBuffer,6,I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    err=i2c_master_cmd_begin(I2C_NUM_0,cmd,100);
    if(err!=ESP_OK) {ESP_LOGE(TAG,"in read data:%s",esp_err_to_name(err));return false;}
    i2c_cmd_link_delete(cmd);

    //数据处理
    uint16_t temperature,humidity;
    temperature=((resultBuffer[0]<<8)|resultBuffer[1]);
    humidity   =((resultBuffer[3]<<8)|resultBuffer[4]);

    //CRC校验
    //float finalTemp,finalHumi;
    if(CRC_Check(resultBuffer,2,resultBuffer[2]) && CRC_Check(resultBuffer+3,2,resultBuffer[5])){
        *temp=temperature/65535.0*175-45;
        *humi=humidity/65535.0*100;

        ESP_LOGI(TAG,"temp:%.1f humi:%.1f",*temp,*humi);
        return true;
    }else{
        
    }
    return false;
}
void SHT3x_Init(){
    I2C_Init();
    SHT3x_WriteCommand(0x30a2,true);//软件复位
    vTaskDelay(10);
    SHT3x_WriteCommand(0x2130,false);     //设置周期性获取数据
    vTaskDelay(10);
    
}

////////////以下非BSP部分，方便分类把Task_SHT3x也放在下面/////////////
#include "lvgl.h"
#include "mainscene.h"

extern SemaphoreHandle_t xGuiSemaphore;

void Task_SHT3x(void *args)
{
    float temperature,humidity;
    SHT3x_Init();

    while (1)
    {
        vTaskDelay(1000);
        if (SHT3x_ReadTempAndHumid(&temperature,&humidity))
        {
            char buf[10];
            
            xSemaphoreTake(xGuiSemaphore,portMAX_DELAY);
            /*set_text_fmt读不了float*/
            sprintf(buf,"%.1f°C",temperature);
            //printf("%s",buf);
            lv_label_set_text(lv_obj_get_child(obj_temperature,1),buf);
            sprintf(buf,"%.1f%%RH",humidity);
            //printf("%s",buf);
            lv_label_set_text(lv_obj_get_child(obj_temperature,2),buf);
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    
}
