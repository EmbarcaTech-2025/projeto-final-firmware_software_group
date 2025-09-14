#include <stdio.h> // Biblioteca padrão
#include <stdlib.h>
#include <string.h>
#include <math.h>

// PICO standard libraries  
#include "pico/stdlib.h" 
#include "pico/cyw43_arch.h" 
#include "hardware/gpio.h" 
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"

// driver for the motor (h bridge tb6612fng)
#include "include/motor.h"
#include "include/mpu6050_i2c.h"
#include "include/mqtt_comm.h"
#include "include/wifi_conn.h"

#define RED_LED 13
#define GREEN_LED 11
#define BLUE_LED 12

#define LEFT_BUTTON 6
#define RIGHT_BUTTON 5

struct Angle {
    float x, y, z;
};
typedef struct Angle Angle_t;

struct Gyro {
    float x, y, z;
};
typedef struct Gyro Gyro_t;


struct Acc {
    float x, y, z;
};
typedef struct Acc Acc_t;


void button_setup() {
    gpio_init(LEFT_BUTTON);
    gpio_init(RIGHT_BUTTON);

    gpio_set_dir(LEFT_BUTTON, GPIO_IN);
    gpio_set_dir(RIGHT_BUTTON, GPIO_IN);

    gpio_pull_up(LEFT_BUTTON);
    gpio_pull_up(RIGHT_BUTTON);

}

void leds_setup() {
    gpio_init(RED_LED);
    gpio_init(GREEN_LED);
    gpio_init(BLUE_LED);

    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_put(RED_LED, 0);

    gpio_set_dir(GREEN_LED, GPIO_OUT);
    gpio_put(GREEN_LED, 0);

    gpio_set_dir(BLUE_LED, GPIO_OUT);
    gpio_put(BLUE_LED, 0);
}

void set_leds(uint red_signal, uint green_signal, uint blue_signal) {
    gpio_put(RED_LED, red_signal);
    gpio_put(GREEN_LED, green_signal);
    gpio_put(BLUE_LED, blue_signal);

}


int main() {
    bool green_signal = false, red_signal = false;
    int delay_time_ms, acumulated_delay_time_ms, max_delay_time_ms;
    int direction;
    float control_signal, control_signal_percentage;
    float magnitude;
    float limitado;
    int16_t level;
    int16_t level_left=0, level_right=0;
    float offset;
    int debug= 0;
    float roll, pitch, yaw;

    int16_t accel_raw[3], gyro_raw[3], temp;
    float accel[3], gyro[3], angles[3];
    Angle_t angle = {0,0,0};
    Gyro_t gyroscope;
    Acc_t accelerometer;


    stdio_init_all();

    sleep_ms(3000);
    printf("Welcome to the binary world!\n");

    // setup the buttons
    button_setup();

    // setup the leds
    leds_setup();
    
    // setup and enable motor
    motor_setup();
    motor_enable();

    // setup and enable the mpu6050
    mpu6050_setup_i2c();
    mpu6050_reset();

    /**
    control_signal = +0.50f * 255.0f;
    direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level = (uint16_t)limitado << 8; // converte e ajusta escala
    **/

    control_signal_percentage = 0.20;

    control_signal = control_signal_percentage * 255.0f;
    direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level_left = (uint16_t)limitado << 8; // converte e ajusta escala

    offset = 30; // in percentage
    //control_signal = control_signal_percentage*(1-offset/100.0) * 255.0f;
    control_signal = control_signal_percentage * 255.0f;
    direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level_right = (uint16_t)limitado << 8; // converte e ajusta escala



    debug = 0;
    if (debug) {
        level_left = 0;
       level_right = 0;
    }

    delay_time_ms = 100;
    acumulated_delay_time_ms = 0;
    max_delay_time_ms = 1000;
	while(1) {

        // control for the direction using the accelerometer/gyroscope
        mpu6050_read_raw(accel_raw, gyro_raw, &temp);

        // Convert to g's and degrees per second
        for (uint8_t i = 0; i < 3; i++) {
            accel[i] = (float)accel_raw[i] / 16384.0f;
            gyro[i] = (float)gyro_raw[i] / 131.0f;
            //angles[i] = (180*asinf(accel[i]))/M_PI;
        }
        /**
        angle.x = angles[0];
        angle.y = angles[1];
        angle.z = angles[2];
        **/

        accelerometer.x = accel[0]; 
        accelerometer.y = accel[1]; 
        accelerometer.z = accel[2];
        gyroscope.x = gyro[0];
        gyroscope.y = gyro[1];
        gyroscope.z = gyro[2];
        roll = (atan2(accelerometer.y, accelerometer.z)*180)/(M_PI);
        pitch = (atan2(-accelerometer.x, 
            sqrt(accelerometer.z*accelerometer.z  +accelerometer.y*accelerometer.y))
            *180)/(M_PI);
        yaw = (atan2(accelerometer.x, accelerometer.y)*180)/(M_PI);


        //direction = angle.y < 0;

        /*
        printf("--------------------------------\n");
        printf("Accel X: %.3f g's, Y: %.3f g's, Z: %.3f g's \n", accel[0], accel[1], accel[2]);
        printf("Gyro X: %.1f °/s Y: %.1f °/s, Z: %.1f °/s\n", gyro[0], gyro[1], gyro[2]);
        printf("Gyro X: %.1f °, Y: %.1f °, Z: %.1f °\n", angle.x, angle.y, angle.z);

        printf("\n\n");
        */

        // motor_set_both_level(level, direction);  
        // motor_set_left_level(level_left, direction);
        // motor_set_right_level(level_right, direction);

        // sleep_ms(delay_time_ms);


        // motor_set_both_level(0, direction);

        // right button start the motors
        if (!gpio_get(RIGHT_BUTTON)) {
            printf("right button pressed!\n");
            green_signal = true;
            red_signal = false;
            set_leds( 0, 1, 0);
            motor_set_left_level(level_left, direction);
            motor_set_right_level(level_right, direction);

            printf("the level left is %d\n", level_left);
            printf("the level right is %d\n", level_right);
        }

        // left button stop the motors
        if (!gpio_get(LEFT_BUTTON)) {
            printf("left button pressed!\n");
            green_signal = false;
            red_signal = true;
            set_leds( 1, 0, 0);
            motor_set_left_level(0, direction);
            motor_set_right_level(0, direction);
        }

        sleep_ms(delay_time_ms);



        acumulated_delay_time_ms+=delay_time_ms;
        /*
        if (acumulated_delay_time_ms >= max_delay_time_ms) {
            acumulated_delay_time_ms = 0;
            //printf("Angle x: °%.1f\n", angle.x);
            //printf("Angle y: %.1f\n", angle.x);
            //printf("Angle z: %.1f\n", angle.x);

            printf("roll: °%.1f\n", roll);
            printf("pitch: °%.1f\n", pitch);
            printf("yaw: °%.1f\n", yaw);
            printf("Gyro X: %.1f °/s Y: %.1f °/s, Z: %.1f °/s\n", gyro[0], gyro[1], gyro[2]);


            printf("\n\n");

        }
        */
        
    }

    return 0;
}
