#ifndef _SHT3X_
#define _SHT3X_

#ifdef __cplusplus
extern "C"{
#endif

#define SHT30_ADDR 0x44

#define I2C_WR 0
#define I2C_RD 1
#define I2C_SDA_GPIO GPIO_NUM_26
#define I2C_SCL_GPIO GPIO_NUM_25
#define I2C_MASTER_FREQ 100000      //less than 1MHz

void Task_SHT3x(void* args);

#ifdef __cplusplus
}
#endif

#endif