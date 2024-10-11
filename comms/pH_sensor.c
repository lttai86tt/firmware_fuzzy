#include "pH_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

int file;

int i2c_init(char* filename) {
    int file;
    if ((file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        exit(1);
    }

    if (ioctl(file, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }
    return file;
}

void ads1115_configure() {
    char config[3] = {0};
    config[0] = CONFIG_REG;
    config[1] = 0xC2; 
    config[2] = 0x83; 
    write(file, config, 3);
}

void getpH() {
    char reg[1] = {CONVERSION_REG};
    write(file, reg, 1);
    usleep(8000);

    char data[2] = {0};
    if (read(file, data, 2) != 2){
        perror("Failed to read from the i2c bus");
        exit(1);
    }else{
        int raw_adc = (data[0] << 8) | data[1];
        if (raw_adc > 0x7FFF) {
            raw_adc -= 0x10000;
        }
        float voltage = raw_adc * 4.096 / 32768.0;
        float pH_val = (voltage - 2.5) * 3.5;
        float abs_pH = fabs(pH_val);
        return pH_val;
    }
}
