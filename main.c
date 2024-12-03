
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include "driver/ledc.h"
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <wiringPi.h>

#define LED_PIN     26


#define ADS1115_ADDRESS 0x48
#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00
#define CONFIG_MUX_SINGLE_0 0x4000  
#define CONFIG_GAIN_ONE 0x0200   
#define CONFIG_MODE_SINGLE 0x0100   
#define CONFIG_START_SINGLE 0x8000 
#define CONFIG_DR_128SPS 0x0080    
#define CONFIG_COMP_QUE_DISABLE 0x0003 
#define MAXNAME 10
#define UPPER_LIMIT 255

int max(int a, int b);
int min(int a, int b);

typedef struct mf_type {
    char name[MAXNAME]; 
    int value; 
    int point1; 
    int point2; 
    int slope1;    
    int slope2;    
    struct mf_type *next;
} MFType;

typedef struct io_type {
    char name[MAXNAME];
    int value;
    MFType *membership_functions;
    struct io_type *next;
} IOType;

IOType *System_Inputs = NULL;  
IOType *System_Outputs = NULL;


typedef struct rule_element_type {
    int *value;  
    struct rule_element_type *next; 
} RuleElementType;

typedef struct rule_type {
    RuleElementType *if_side;
    RuleElementType *then_side;
    struct rule_type *next;
} RuleType;

RuleType *Rule_Base;


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


int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void initPWM() {
    // Cấu hình kênh PWM cho van acid
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,                       
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 1,              //           
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);

        // Cấu hình kênh PWM cho van bazo
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,                       
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 1,              //           
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
        // Cấu hình kênh PWM cho peltier nong
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,                       
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 1,              //           
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
        // Cấu hình kênh PWM cho peltier lanh
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,                       
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 1,              //           
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void get_system_inputs() {
    IOType *sensor_input = System_Inputs;
    while(sensor_input != NULL) {
        if(strcmp(sensor_input->name, "pH") == 0) {
            sensor_input->value = getpH();
        } else if(strcmp(temp_input->name, "Temperature") == 0) {
            sensor_input->value = getTemperature();
        }
        sensor_input = sensor_input->next;
    }
}

/*
  Compute the degree of menbership of the input to the membership function

  Mức độ thành viên (y)
    1 |     ________
      |    /        \
      |   /          \
      |  /            \
      | /              \
    0 |/_____________________________
      0   point1    point2    Giá trị đầu vào (x)
*/
void compute_degree_of_membership(MFType *mf, int input) {
    int delta_1;
    int delta_2;
    delta_1 = input - mf->point1;
    delta_2 = mf->point2 - input;

    if ((delta_1 <= 0) || (delta_2 <= 0))
        mf->value = 0;
    else
        mf->value = min((mf->slope1 * delta_1), (mf->slope2 * delta_2));
    
    mf->value = min(mf->value, UPPER_LIMIT);
}

// Compute the area of the trapezoid
int compute_area_of_trapezoid(MFType *mf) {
    int run_1;
    int run_2;
    int base;
    int top;
    int area;

    base = mf->point2 - mf->point1;
    run_1 = mf->value / mf->slope1;
    run_2 = mf->value / mf->slope2;
    top = base - run_1 - run_2;

    if (top < 0)  // Ensure top is not negative
        top = 0;

    area = mf->value * (base + top) / 2;
    return area > 0 ? area : 0;  // Ensure area non-negative
}

/*
 Initialize the system inputs and their membership functions
 Check the slope again 
*/
void initialize_system() {
    IOType *pH = malloc(sizeof(IOType));
    strcpy(pH->name, "pH"); //
    pH->value = 0;
    
    MFType *low = malloc(sizeof(MFType));
    strcpy(low->name, "Low");
    low->point1 = 0; low->point2 = 7;
    low->slope1 = 1; low->slope2 = 1;
    
    MFType *high = malloc(sizeof(MFType));
    strcpy(high->name, "High");
    high->point1 = 8; high->point2 = 14;
    high->slope1 = 1; high->slope2 = 1;
    
    MFType *normal_ph = malloc(sizeof(MFType));
    strcpy(normal_ph->name, "Normal");
    normal_ph->point1 = 7; normal_ph->point2 = 8;
    normal_ph->slope1 = 1; normal_ph->slope2 = 1;

    low->next = normal_ph;
    normal_ph->next = high;
    high->next = NULL;

    pH->membership_functions = low;
    pH->next = NULL;

    System_Inputs = pH;

    IOType *temperature = malloc(sizeof(IOType));
    strcpy(temperature->name, "Temperature");
    temperature->value = 0;

    MFType *cold = malloc(sizeof(MFType));
    strcpy(cold->name, "Cold");
    cold->point1 = 0; cold->point2 = 25;
    cold->slope1= 1; cold->slope2 = 1;

    MFType *hot = malloc(sizeof(MFType));
    strcpy(hot->name, "Hot");
    hot->point1 = 30; hot->point2 = 40;
    hot->slope1 = 1; hot->slope2 = 1;

    MFType *normal_temp = malloc(sizeof(MFType));
    strcpy(normal_temp->name, "Normal");
    normal_temp->point1 = 25; normal_temp->point2 = 30;
    normal_temp->slope1 = 1; normal_temp->slope2 = 1;

    cold->next = normal_temp;
    normal_temp->next = hot;
    hot->next = NULL;

    temperature->membership_functions = cold;
    temperature->next = NULL;

    pH->next = temperature;

    // Initialize outputs (e.g., valve acid and valve bazo) and their membership functions

    // Outputs initialization code similar to inputs would follow here...
} 

void fuzzification() {
    IOType *si;
    MFType *mf;
    for(si=System_Inputs; si != NULL; si=si->next)
        for(mf=si->membership_functions; mf != NULL; mf=mf->next)
            compute_degree_of_membership(mf, si->value);
}

void rule_evaluation(){
    RuleType *rule;
    RuleElementType *ifp; 
    RuleElementType *thenp; 
    int strength;

    for (rule = Rule_Base; rule != NULL; rule = rule->next) {
        strength = UPPER_LIMIT;
        for (ifp = rule->if_side; ifp != NULL; ifp = ifp->next)
            strength = min(strength, *(ifp->value));

        for (thenp = rule->then_side; thenp != NULL; thenp = thenp->next)
            *(thenp->value) = max(strength, *(thenp->value));
    }
}

void defuzzification(){
    // Imagine System_Outputs contains 4 output system
    IOType *so;
    MFType *mf;
    int index = 0;
    
    int defuzzifiedOutputs[4] = {0}; 

    for(so = System_Outputs; so != NULL; so=so->next, index++) {
        int sum_of_products = 0;
        int sum_of_areas = 0;
        for(mf = so->membership_functions; mf != NULL; mf = mf->next) {
             int area = compute_area_of_trapezoid(mf);
             int centroid = mf->point1 + (mf->point2 - mf->point1) / 2;
             sum_of_products += area * centroid;
             sum_of_areas += area;
        }

        so->value = sum_of_areas != 0 ? sum_of_products / sum_of_areas : 0;
        so->value = min(max(so->value, 0), 8191); 

        defuzzifiedOutputs[index] = so->value;
    }
    controlDevices(defuzzifiedOutputs[0], defuzzifiedOutputs[1], defuzzifiedOutputs[2], defuzzifiedOutputs[3]);
}

void put_system_outputs() {
    IOType *output;
    for (output = System_Outputs; output != NULL; output = output->next) {
        // Print output. Adjusting up-to requires the system.
        printf("Output %s: %d\n", output->name, output->value);
    }
}

void controlDevices(int valveAcidOutput, int valveBazoOutput, int heaterOutput, int coolerOutput) {
    if (valveAcidOutput > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, valveAcidOutput);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    } else {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    }

    if (valveBazoOutput > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, valveBazoOutput);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    }

    if (heaterOutput > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, heaterOutput);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
    }

    if (coolerOutput > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, coolerOutput);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3);
    }
}

void fuzzy_main(){
    while(1){
        fuzzification();
        rule_evaluation();
        defuzzification();

        int valveAcidOutput = 0;
        int valveBazoOutput = 0;
        int heaterOutput = 0;
        int coolerOutput = 0;

        put_system_outputs();

        controlDevices(
            System_Outputs->value,           
            System_Outputs->next->value,    
            System_Outputs->next->next->value, 
            System_Outputs->next->next->next->value 
        );
    }
}

int main() {

    int file = i2c_init((char*)"/dev/i2c-1");
    ads1115_configure(file);
    //getTemperature();

    initialize_system();
    fuzzy_main();

}

