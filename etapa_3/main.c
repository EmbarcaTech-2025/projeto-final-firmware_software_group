#include <stdio.h> // Biblioteca padrão
#include <stdlib.h>
#include <string.h>

// PICO standard libraries  
#include "pico/stdlib.h" 
#include "pico/cyw43_arch.h" 
#include "hardware/gpio.h" 
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"

// driver for the motor (h bridge tb6612fng)
#include "motor.h"

int main() {

    stdio_init_all();
    sleep_ms(3000);
    printf("Welcome to the binary world!\n");
    
    // setup and enable motor
    motor_setup();
    motor_enable();

	while(1) {

        printf("testing\n");

        
    }

    return 0;
}
