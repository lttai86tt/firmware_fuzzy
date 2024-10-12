#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#define ADS1115_ADDRESS 0x48
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

#define CONFIG_MUX_SINGLE_0 0x4000  
#define CONFIG_GAIN_ONE 0x0200   
#define CONFIG_MODE_SINGLE 0x0100   
#define CONFIG_START_SINGLE 0x8000 
#define CONFIG_DR_128SPS 0x0080    
#define CONFIG_COMP_QUE_DISABLE 0x0003 

int i2c_init(char* filename);
void ads1115_configure();
float getpH();

#endif 
