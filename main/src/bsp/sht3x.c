#include "sht3x.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

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

void Task_SHT3x(void *args)
{

}
