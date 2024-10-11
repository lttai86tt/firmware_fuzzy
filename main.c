#include "pH_sensor.h"
#include "temp_sensor.h"
#include "fuzzy_rule.h"
#include <stdio.h>
#include <unistd.h>

int main() {

    int file = i2c_init((char*)"/dev/i2c-1");
    ads1115_configure(file);
    //getTemperature();

    initialize_system();
    fuzzy_main();

}

