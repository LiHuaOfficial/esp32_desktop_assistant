#ifndef _SHT3X_
#define _SHT3X_

#ifdef __cplusplus
extern "C"{
#endif

#define I2C_SDA_GPIO GPIO_NUM_35
#define I2C_SCL_GPIO GPIO_NUM_34
#define I2C_MASTER_FREQ 100000      //less than 1MHz

void Task_SHT3x(void* args);

#ifdef __cplusplus
}

#endif