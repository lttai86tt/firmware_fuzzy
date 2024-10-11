#ifndef FUZZY_COLTROL_H
#define FUZZY_COLTROL_H

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

typedef struct rule_element_type {
    int *value;  
    struct rule_element_type *next; 
} RuleElementType;

typedef struct rule_type {
    RuleElementType *if_side;
    RuleElementType *then_side;
    struct rule_type *next;
} RuleType;

void initialize_system();
void fuzzification();
void rule_evaluation();
void defuzzification();
void put_system_outputs();
void compute_degree_of_membership(MFType *mf, int input);
void controlDevices(int valveAcidOutput, int valveBazoOutput, int heaterOutput, int coolerOutput);
void turnOnRelay(int relayPin);
void turnOffRelay(int relayPin);

void get_system_inputs();
void fuzzy_main();

#endif FUZZY_COLTROL_H
