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
#include "motor.h"

int main() {
    int direction;
    float control_signal;
    float magnitude;
    float limitado;
    int16_t level;

    stdio_init_all();
    sleep_ms(3000);
    printf("Welcome to the binary world!\n");
    
    // setup and enable motor
    motor_setup();
    motor_enable();

    control_signal = +0.50f * 255.0f;
    direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level = (uint16_t)limitado << 8; // converte e ajusta escala

    printf("the level is %d\n", level);
    level = 0;
	while(1) {
        motor_set_both_level(level, direction);
        sleep_ms(3000);
        printf("testing\n");
        printf("the level is %d\n", level);

        motor_set_both_level(0, direction);
        sleep_ms(3000);

        
    }

    return 0;
}
