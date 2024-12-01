#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Định nghĩa GPIO
#define PELTIER_PWM_PIN 1 // GPIO18 tương ứng với WiringPi Pin 1
#define TEMP_SENSOR "/sys/bus/w1/devices/28-XXXXXXXXXXXX/w1_slave" // Thay "28-XXXXXXXXXXXX" bằng ID cảm biến DS18B20

// Cấu hình fuzzy logic
float error_negative(float error) { return fmax(0.0, fmin(1.0, (0.0 - error) / 5.0)); }
float error_zero(float error) { return fmax(0.0, 1.0 - fabs(error) / 5.0); }
float error_positive(float error) { return fmax(0.0, fmin(1.0, (error - 0.0) / 5.0)); }
float output_low(float output) { return fmax(0.0, fmin(1.0, (50.0 - output) / 50.0)); }
float output_medium(float output) { return fmax(0.0, 1.0 - fabs(output - 50.0) / 50.0); }
float output_high(float output) { return fmax(0.0, fmin(1.0, (output - 50.0) / 50.0)); }

// Fuzzy logic
float fuzzyControl(float error, float deltaError) {
    // Tính membership
    float e_neg = error_negative(error);
    float e_zero = error_zero(error);
    float e_pos = error_positive(error);
    
    float de_neg = error_negative(deltaError);
    float de_zero = error_zero(deltaError);
    float de_pos = error_positive(deltaError);
    
    // Áp dụng quy tắc fuzzy
    float rule_high = fmin(e_neg, de_neg);
    float rule_medium = fmin(e_zero, de_zero);
    float rule_low = fmin(e_pos, de_pos);
    
    // Kết hợp các kết quả
    float aggregated = (rule_high * 100.0 + rule_medium * 50.0 + rule_low * 0.0) /
                       (rule_high + rule_medium + rule_low);
    return isnan(aggregated) ? 0.0 : aggregated;
}

int main() {
    // Khởi tạo GPIO
    if (wiringPiSetup() == -1) {
        printf("Không thể khởi tạo WiringPi!\n");
        return 1;
    }
    pinMode(PELTIER_PWM_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(1024);  // Giá trị PWM tối đa
    pwmSetClock(384);  // Tần số khoảng 1kHz
    
    float targetTemp = 25.0;  // Nhiệt độ mong muốn
    float lastError = 0.0;
    
    while (1) {
        float currentTemp = readTemperature();
        if (currentTemp < -100) {
            printf("Lỗi cảm biến nhiệt độ!\n");
            continue;
        }

        // Tính toán error và deltaError
        float error = targetTemp - currentTemp;
        float deltaError = error - lastError;

        // Áp dụng fuzzy logic
        float pwmValue = fuzzyControl(error, deltaError);
        int pwmDuty = (int)((pwmValue / 100.0) * 1024); // Chuyển về giá trị 0-1024
        pwmWrite(PELTIER_PWM_PIN, pwmDuty);

        printf("Nhiệt độ hiện tại: %.2f°C, PWM: %d/1024\n", currentTemp, pwmDuty);
        lastError = error;
        delay(1000);  // Chờ 1 giây
    }

    return 0;
}
