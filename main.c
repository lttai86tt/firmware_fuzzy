

/*
* PWM init for controller
* Add pH sensor input
* Add PWM Control on each device (Valve + Peltier)
* (Unsure)  fuzzy_main
* (Unsure)  Initialize the system inputs and their membership functions
* Compute the degree of menbership of the input to the membership function

  Mức độ thành viên (y)
    1 |     ________
      |    /        \
      |   /          \
      |  /            \
      | /              \
    0 |/_____________________________
      0   point1    point2    Giá trị đầu vào (x)

*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <softPwm.h>
#include <wiringPi.h>

/* Need to be configured after */
#define GPIO_VALVE_ACID 1 // GPIO cho van acid
#define GPIO_VALVE_BAZO 2 // GPIO cho van bazo
#define GPIO_HEATER 3     // GPIO cho hot peltier
#define GPIO_COOLER 4     // GPIO cho cool peltier

/* Use for pH sensor */
// #define ADS1115_ADDRESS 0x48
// #define CONFIG_REG 0x01
// #define CONVERSION_REG 0x00
// #define CONFIG_MUX_SINGLE_0 0x4000
// #define CONFIG_GAIN_ONE 0x0200
// #define CONFIG_MODE_SINGLE 0x0100
// #define CONFIG_START_SINGLE 0x8000
// #define CONFIG_DR_128SPS 0x0080
// #define CONFIG_COMP_QUE_DISABLE 0x0003

#define MAXNAME 10
#define UPPER_LIMIT 255

int max(int a, int b);
int min(int a, int b);

typedef struct mf_type
{
    char name[MAXNAME];
    int value;
    int point1;
    int point2;
    int slope1;
    int slope2;
    struct mf_type *next;
} MFType;

typedef struct io_type
{
    char name[MAXNAME];
    float value;
    MFType *membership_functions;
    struct io_type *next;
} IOType;

typedef struct rule_element_type
{
    int *value;
    struct rule_element_type *next;
} RuleElementType;

typedef struct rule_type
{
    RuleElementType *if_side;
    RuleElementType *then_side;
    struct rule_type *next;
} RuleType;

RuleType *Rule_Base = NULL;
IOType *System_Inputs = NULL;
IOType *System_Outputs = NULL;

int valveAcidOutput = 0;
int valveBazoOutput = 0;
int heaterOutput = 0;
int coolerOutput = 0;

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

float get_Temperature()
{

    const char *sensor_temperature = "/sys/bus/w1/devices/28-3ce1d4434496/w1_slave";
    FILE *fp;
    char buf[256];
    char *temp_str;
    float temp;

    if ((fp = fopen(sensor_temperature, "r")) == NULL)
    {
        perror("Failed to open sensor file");
        return -1;
    }

    if (fread(buf, sizeof(char), 256, fp) < 1)
    {
        perror("Failed to read sensor file");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    if ((temp_str = strstr(buf, "t=")) == NULL)
    {
        fprintf(stderr, "Failed to find temperature in sensor file\n");
        return -1;
    }

    temp_str += 2;
    temp = strtof(temp_str, NULL);
    return temp / 1000;
}

/* Missing get_pHsensor*/
float get_pHsensor()
{
}

/* UNSURE: Need to check if this is correct */
void initGPIO()
{
    wiringPiSetup();

    pinMode(GPIO_VALVE_ACID, OUTPUT);
    pinMode(GPIO_VALVE_BAZO, OUTPUT);
    pinMode(GPIO_HEATER, OUTPUT);
    pinMode(GPIO_COOLER, OUTPUT);

    softPwmCreate(GPIO_VALVE_ACID, 0, 100);
    softPwmCreate(GPIO_VALVE_BAZO, 0, 100);
    softPwmCreate(GPIO_HEATER, 0, 100);
    softPwmCreate(GPIO_COOLER, 0, 100);
}

// get input value on to the program
void get_system_inputs()
{
    IOType *sensor_input = System_Inputs;
    while (sensor_input != NULL)
    {
        if (strcmp(sensor_input->name, "Temperature") == 0)
        {
            sensor_input->value = get_Temperature();
        }
        else if (strcmp(sensor_input->name, "pH") == 0)
        {
            // sensor_input->value = get_pHsensor();
        }
        sensor_input = sensor_input->next;
    }
}

void compute_degree_of_membership(MFType *mf, int input)
{
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
int compute_area_of_trapezoid(MFType *mf)
{
    int run_1;
    int run_2;
    int base;
    int top;
    int area;

    base = mf->point2 - mf->point1;
    run_1 = mf->value / mf->slope1;
    run_2 = mf->value / mf->slope2;
    top = base - run_1 - run_2;

    if (top < 0)
        top = 0;

    area = mf->value * (base + top) / 2;
    return area > 0 ? area : 0;
}

// precise numerical values
void fuzzification()
{
    IOType *si;
    MFType *mf;
    for (si = System_Inputs; si != NULL; si = si->next)
        for (mf = si->membership_functions; mf != NULL; mf = mf->next)
            compute_degree_of_membership(mf, si->value);
}

void rule_evaluation()
{
    RuleType *rule;
    RuleElementType *ifp;
    RuleElementType *thenp;
    int strength;

    for (rule = Rule_Base; rule != NULL; rule = rule->next)
    {
        strength = UPPER_LIMIT;
        for (ifp = rule->if_side; ifp != NULL; ifp = ifp->next)
            strength = min(strength, *(ifp->value));

        for (thenp = rule->then_side; thenp != NULL; thenp = thenp->next)
            *(thenp->value) = max(strength, *(thenp->value));
    }
}

void defuzzification()
{
    // Imagine System_Outputs contains 4 output system
    IOType *so;
    MFType *mf;
    int index = 0;

    int defuzzifiedOutputs[4] = {0};

    for (so = System_Outputs; so != NULL; so = so->next, index++)
    {
        int sum_of_products = 0;
        int sum_of_areas = 0;
        for (mf = so->membership_functions; mf != NULL; mf = mf->next)
        {
            int area = compute_area_of_trapezoid(mf);
            int centroid = mf->point1 + (mf->point2 - mf->point1) / 2;
            sum_of_products += area * centroid;
            sum_of_areas += area;
        }

        so->value = sum_of_areas != 0 ? sum_of_products / sum_of_areas : 0;
        so->value = min(max(so->value, 0), 100);

        defuzzifiedOutputs[index] = so->value;
    }
    controlDevices(defuzzifiedOutputs[0], defuzzifiedOutputs[1], defuzzifiedOutputs[2], defuzzifiedOutputs[3]);
}

void put_system_outputs()
{
    IOType *output;
    for (output = System_Outputs; output != NULL; output = output->next)
    {
        printf("Output %s: %d\n", output->name, output->value);
    }
}

int clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

/*
 * Unsure: need to check that it controls devices
 * pwm signal level control
 */
void controlDevices(int valveAcidOutput, int valveBazoOutput, int heaterOutput, int coolerOutput)
{
    softPwmWrite(GPIO_VALVE_ACID, clamp(valveAcidOutput, 0, 100));
    softPwmWrite(GPIO_VALVE_BAZO, clamp(valveBazoOutput, 0, 100));
    softPwmWrite(GPIO_HEATER, clamp(heaterOutput, 0, 100));
    softPwmWrite(GPIO_COOLER, clamp(coolerOutput, 0, 100));

    printf("Valve Acid PWM: %d, Valve Bazo PWM: %d, Heater PWM: %d, Cooler PWM: %d\n",
           valveAcidOutput, valveBazoOutput, heaterOutput, coolerOutput);
}

/*
 Initialize the system inputs and their membership functions
 Check the slope again
*/
void initialize_system()
{
    // INPUT: Temperature rule
    IOType *temperature = malloc(sizeof(IOType));
    strcpy(temperature->name, "Temperature");
    temperature->value = 0;

    MFType *cold = malloc(sizeof(MFType));
    strcpy(cold->name, "Cold");
    cold->point1 = 0; cold->point2 = 25;
    cold->slope1 = 1; cold->slope2 = 1;

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
    System_Inputs = temperature;

    // INPUT: pH rule
    /*

    */

    // OUTPUT: Valve Acid rule
    IOType *valveAcid = malloc(sizeof(IOType));
    strcpy(valveAcid->name, "ValveAcid");
    valveAcid->value = 0;

    MFType *lowAcid = malloc(sizeof(MFType));
    strcpy(lowAcid->name, "LowAcid");
    lowAcid->point1 = 0; lowAcid->point2 = 50;
    lowAcid->slope1 = 1; lowAcid->slope2 = 1;

    MFType *highAcid = malloc(sizeof(MFType));
    strcpy(highAcid->name, "HighAcid");
    highAcid->point1 = 50; highAcid->point2 = 100;
    highAcid->slope1 = 1; highAcid->slope2 = 1;

    lowAcid->next = highAcid;
    highAcid->next = NULL;

    valveAcid->membership_functions = lowAcid;
    valveAcid->next = NULL;
    //System_Outputs = valveAcid;

    // OUTPUT: Valve Bazo rule
    IOType *valveBazo = malloc(sizeof(IOType));
    strcpy(valveBazo->name, "ValveBazo");
    valveBazo->value = 0;

    MFType *lowBazo = malloc(sizeof(MFType));
    strcpy(lowBazo->name, "LowBazo");
    lowBazo->point1 = 0; lowBazo->point2 = 50;
    lowBazo->slope1 = 1; lowBazo->slope2 = 1;

    MFType *highBazo = malloc(sizeof(MFType));
    strcpy(highBazo->name, "HighBazo");
    highBazo->point1 = 50; highBazo->point2 = 100;
    highBazo->slope1 = 1; highBazo->slope2 = 1;
 
    lowBazo->next = highBazo;
    highBazo->next = NULL;

    valveBazo->membership_functions = lowBazo;
    valveBazo->next = valveAcid;
    //System_Outputs = valveBazo;

    // OUTPUT: Heater peltier rule
    IOType *heater = malloc(sizeof(IOType));
    strcpy(heater->name, "Heater");
    heater->value = 0;

    MFType *lowHeat = malloc(sizeof(MFType));
    strcpy(lowHeat->name, "LowHeat");
    lowHeat->point1 = 0; lowHeat->point2 = 50;
    lowHeat->slope1 = 1; lowHeat->slope2 = 1;

    MFType *highHeat = malloc(sizeof(MFType));
    strcpy(highHeat->name, "HighHeat");
    highHeat->point1 = 50; highHeat->point2 = 100;
    highHeat->slope1 = 1; highHeat->slope2 = 1;

    lowHeat->next = highHeat;
    highHeat->next = NULL;

    heater->membership_functions = lowHeat;
    heater->next = valveBazo;
    //System_Outputs = heater;

    // OUTPUT: Cooler peltier rule
    IOType *cooler = malloc(sizeof(IOType));
    strcpy(cooler->name, "Cooler");
    cooler->value = 0;

    MFType *lowCool = malloc(sizeof(MFType));
    strcpy(lowCool->name, "LowCool");
    lowCool->point1 = 0; lowCool->point2 = 50;
    lowCool->slope1 = 1; lowCool->slope2 = 1;

    MFType *highCool = malloc(sizeof(MFType));
    strcpy(highCool->name, "HighCool");
    highCool->point1 = 50; highCool->point2 = 100;
    highCool->slope1 = 1; highCool->slope2 = 1;

    lowCool->next = highCool;
    highCool->next = NULL;

    cooler->membership_functions = lowCool;
    cooler->next = heater;
    System_Outputs = cooler;

    // RULE: Nếu Temperature là Cold thì bật Heater
    RuleType *rule1 = malloc(sizeof(RuleType));
    RuleElementType *if_temp_cold = malloc(sizeof(RuleElementType));
    RuleElementType *then_heater_on = malloc(sizeof(RuleElementType));

    if_temp_cold->value = cold;
    if_temp_cold->next = NULL;

    then_heater_on->value = lowHeat;
    then_heater_on->next = NULL;

    rule1->if_side = if_temp_cold;
    rule1->then_side = then_heater_on;

    // RULE: Nếu Temperature là Hot thì bật Cooler
    RuleType *rule2 = malloc(sizeof(RuleType));
    RuleElementType *if_temp_hot = malloc(sizeof(RuleElementType));
    RuleElementType *then_cooler_on = malloc(sizeof(RuleElementType));

    if_temp_hot->value = hot;
    if_temp_hot->next = NULL;

    then_cooler_on->value = lowCool;
    then_cooler_on->next = NULL;

    rule2->if_side = if_temp_hot;
    rule2->then_side = then_cooler_on;

    // RULE: Nếu pH là Low thì mở Valve Acid
    RuleType *rule3 = malloc(sizeof(RuleType));
    RuleElementType *if_ph_low = malloc(sizeof(RuleElementType));
    RuleElementType *then_valve_acid_on = malloc(sizeof(RuleElementType));

    if_ph_low->value = low_pH;
    if_ph_low->next = NULL;

    then_valve_acid_on->value = lowAcid;
    then_valve_acid_on->next = NULL;

    rule3->if_side = if_ph_low;
    rule3->then_side = then_valve_acid_on;

    // RULE: Nếu pH là High thì mở Valve Bazo
    RuleType *rule4 = malloc(sizeof(RuleType));
    RuleElementType *if_ph_high = malloc(sizeof(RuleElementType));
    RuleElementType *then_valve_bazo_on = malloc(sizeof(RuleElementType));

    if_ph_high->value = high_pH;
    if_ph_high->next = NULL;

    then_valve_bazo_on->value = lowBazo;
    then_valve_bazo_on->next = NULL;

    rule4->if_side = if_ph_high;
    rule4->then_side = then_valve_bazo_on;

    // Kết nối các quy tắc vào danh sách
    rule1->next = rule2;
    rule2->next = rule3;
    rule3->next = rule4;
    rule4->next = NULL;

    Rule_Base = rule1; // Đặt rule1 làm đầu danh sách rule
}

void fuzzy_main()
{
    while (1)
    {
        fuzzification();      // handle input value -> Language variables
        rule_evaluation();    // if-then statement
        defuzzification();    // take the output and convert to crip control signal (can be use to control the system)
        put_system_outputs(); // update on display the current output value (for debug)

        // control devices
        controlDevices(
            clamp(System_Outputs->value, 0, 100),
            clamp(System_Outputs->next->value, 0, 100),
            clamp(System_Outputs->next->next->value, 0, 100),
            clamp(System_Outputs->next->next->next->value, 0, 100));

        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

int main()
{
    initGPIO();
    initialize_system();
    fuzzy_main();
}
