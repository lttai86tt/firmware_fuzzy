#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "fuzzy_rule.h"
#include "comms/pH_sensor.h"
#include "comms/temp_sensor.h"
#include "gpio.h"

RuleType *Rule_Base;
IOType *System_Inputs = NULL;  
IOType *System_Outputs = NULL;

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
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
    cold->= 1; cold->slope2 = 1;

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
    
    /*Setting GPIO*/
    const int acidValveRelayPin = 1;
    const int bazoValveRelayPin = 2;
    const int heaterRelayPin = 3;
    const int coolerRelayPin = 4;

    if (valveAcidOutput > 0) {
        //Call function control realy (ON)
    } else {
        //Off
    }

    if (valveBazoOutput > 0) {

    } else {

    }

    if (heaterOutput > 0) {

    } else {

    }

    if (coolerOutput > 0) {

    } else {

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

        controlDevices(valveAcidOutput, valveBazoOutput, heaterOutput, coolerOutput);
    }
}