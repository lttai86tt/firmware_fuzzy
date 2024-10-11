#include "temp_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void getTemperature() {
    
    const char *sensor_temperature = "/sys/bus/w1/devices/28-3ce1d4434496/w1_slave";
    FILE *fp;
    char buf[256];
    char *temp_str;
    float temp;

    if ((fp = fopen(sensor_file, "r")) == NULL) {
        perror("Failed to open sensor file");
        return;
    }

    if (fread(buf, sizeof(char), 256, fp) < 1) {
        perror("Failed to read sensor file");
        fclose(fp);
        return;
    }

    fclose(fp);

    if ((temp_str = strstr(buf, "t=")) == NULL) {
        fprintf(stderr, "Failed to find temperature in sensor file\n");
        return;
    }

    temp_str += 2;
    temp = strtof(temp_str, NULL);
    temp /= 1000;

    return temp;

    // printf("Temperature: %.3f *C\n", temp);
    // if (temp > 32.0) {
    //     digitalWrite(LED_PIN, HIGH);
    //     delay(200);
    // } else {
    //     digitalWrite(LED_PIN, LOW);
    // }
}
