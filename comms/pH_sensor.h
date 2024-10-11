#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#define ADS1115_ADDRESS 0x48
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

int i2c_init(char* filename);
void ads1115_configure();
float getpH();

#endif 
