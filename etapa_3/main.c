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
#include "include/xor_cipher.h"

#define RED_LED 13
#define GREEN_LED 11
#define BLUE_LED 12

#define LEFT_BUTTON 6
#define RIGHT_BUTTON 5

#define UART_RX_PIN 9
#define UART_TX_PIN 8
#define BAUD_RATE 115200
#define UART_ID uart1

char mqtt_data_buffer[256];
bool start_motor = false, stop_motor = false;

enum Direction {
    left, right, forward, backward, start, stop
};

enum Direction direction = stop;
bool direction_changed = false;

uint with_cryptography = 0;

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


void uart_setup() {
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}


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


void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    uint8_t descriptografada[101];
    uint led_status;

    memset(mqtt_data_buffer, 0, sizeof(mqtt_data_buffer));
    memcpy(mqtt_data_buffer, data, len);

    if (with_cryptography) {
        xor_encrypt((uint8_t *)data, descriptografada, strlen(data), 42);
        //printf("Payload: %.*s\n", len, descriptografada);
        // on_message((char*)arg, descriptografada);


    } else {
        printf("Payload(*data): %s\n", data);
        printf("mqtt_data_buffer: %s\n", mqtt_data_buffer);
        on_message((char*)arg, data);

    }


    // signal the leds
    uint red_led=0, green_led=0, blue_led=0;
    if (strcmp(mqtt_data_buffer, "red_on")==0) {
        red_led = 1;
    } 
    else if (strcmp(mqtt_data_buffer, "red_off")==0) {
        red_led = 0;
    } 
    else if (strcmp(mqtt_data_buffer, "green_on")==0) {
        green_led = 1;
    } 
    else if (strcmp(mqtt_data_buffer, "green_off")==0) {
        green_led = 0;
    } 
    else if (strcmp(mqtt_data_buffer, "blue_on")==0) {
        blue_led = 1;
    } 
    else if (strcmp(mqtt_data_buffer, "blue_off")==0) {
        blue_led = 0;
    } 
    set_leds(red_led, green_led, blue_led);

    // control the start or stop of the motors
    if (strcmp(mqtt_data_buffer, "start_motor") == 0 ) {
        start_motor = true;
    }

    else if (strcmp(mqtt_data_buffer, "stop_motor") == 0) {
        stop_motor = true;
    }


    // control the direction of the motors
    if (strcmp(mqtt_data_buffer, "direction_left") == 0) {
        direction_changed = true;
        direction = left;
    }
    else if (strcmp(mqtt_data_buffer, "direction_right") == 0) {
        direction_changed = true;
        direction = right;
    }
    else if (strcmp(mqtt_data_buffer, "direction_forward") == 0) {
        direction_changed = true;
        direction = forward;
    }
    else if (strcmp(mqtt_data_buffer, "direction_backward") == 0) {
        direction_changed = true;
        direction = backward;
    }
    else if (strcmp(mqtt_data_buffer, "direction_start") == 0) {
        direction_changed = true;
        direction = start;
    }
    else if (strcmp(mqtt_data_buffer, "direction_stop") == 0) {
        direction_changed = true;
        direction = stop;
    }

}


int main() {
    // variaveis de inicializacao
    uint is_subscriber = 1;
    uint is_publisher = 0;
    const char* mqtt_topic = "bitdoglab"; 
    // const char* mqtt_topic = "escola/sala1/temperatura";
    uint xor_key = 42;
    const char *IP = "192.168.15.124";
    const char *USER = "aluno";
    const char *USER_PASSWORD = "senha123";
    char client_id[31];
    char client_subscriber[] = "bitdog_subscriber";
    char client_publisher[] = "bitdog_publisher";
    char SSID[] = "VIVOFIBRA-5598";
    char SSID_PASSWORD[] = "4674B29BC2";



    int delay_time_ms, acumulated_delay_time_ms, max_delay_time_ms;
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


    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID)R e senha
    connect_to_wifi(SSID, SSID_PASSWORD);

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    if (is_subscriber) {
        //mqtt_setup_and_subscribe("bitdog_subscriber", "192.168.151.142", "aluno", "senha123");
        strcpy(client_id, client_subscriber);

        Subscriber_Data arguments_to_subscriber = {
            .function = mqtt_incoming_data_cb,
            .mqtt_topic = mqtt_topic
        };


        mqtt_setup_and_subscribe(client_id, IP, USER, USER_PASSWORD, &arguments_to_subscriber);
    } 
    if (is_publisher) {
        //mqtt_setup_publish("bitdog_publisher", "192.168.151.142", "aluno", "senha123");
        strcpy(client_id, client_publisher);
        mqtt_setup_publish(client_id, IP, USER, USER_PASSWORD);
    }
    
    // Mensagem original a ser enviada
    uint8_t mensagem[101];

    /**
    control_signal = +0.50f * 255.0f;
    direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level = (uint16_t)limitado << 8; // converte e ajusta escala
    **/

    control_signal_percentage = 0.40;

    control_signal = control_signal_percentage * 255.0f;
    // direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level_left = (uint16_t)limitado << 8; // converte e ajusta escala

    offset = 30; // in percentage
    //control_signal = control_signal_percentage*(1-offset/100.0) * 255.0f;
    control_signal = control_signal_percentage * 255.0f;
    // direction = control_signal > 0; // obtem direcao
    magnitude = fabsf(control_signal); // obtem modulo do sinal
    limitado = fminf(magnitude, 255.0f); // limita ao maximo de 255
    level_right = (uint16_t)limitado << 8; // converte e ajusta escala

    delay_time_ms = 100;
    acumulated_delay_time_ms = 0;
    max_delay_time_ms = 1000;

	while(1) {
        if (is_publisher) {
            // Dados a serem enviados
            float payload = 31.2;

            time_t seconds = time(NULL);

            sprintf(mensagem, "{\"valor\":%.2f,\"ts\":%lu}", payload, seconds);
            printf("Mensagem enviada: %s\n", mensagem);

            // Publica a mensagem original (não criptografada)
            //mqtt_comm_publish("escola/sala1/temperatura", mensagem, strlen(mensagem));
            //printf("A mensagem %s foi enviada !!!\n", mensagem);
             // Buffer para mensagem criptografada (16 bytes)
            
            if (with_cryptography) {
                uint8_t criptografada[101];
                // Criptografa a mensagem usando XOR com chave 42
                xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), xor_key);

                
                // Alternativa: Publica a mensagem criptografada (atualmente comentada)
                mqtt_comm_publish(mqtt_topic, criptografada, strlen(mensagem));
            } else {
                mqtt_comm_publish(mqtt_topic, mensagem, strlen(mensagem));
            }
            // wait 5 seconds before the next publishing
            sleep_ms(5000);

        }



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

        // right button start the motors
        if (!gpio_get(RIGHT_BUTTON) || start_motor) {
            printf("right button pressed!\n");
            start_motor = false;
            stop_motor = false;
            set_leds( 0, 1, 0);
            motor_set_left_level(level_left, 1);
            motor_set_right_level(level_right, 1);

            printf("the level left is %d\n", level_left);
            printf("the level right is %d\n", level_right);
        }

        // left button stop the motors
        if (!gpio_get(LEFT_BUTTON) || stop_motor) {
            printf("left button pressed!\n");
            start_motor = false;
            stop_motor = false;;
            set_leds( 1, 0, 0);
            motor_set_left_level(0, 1);
            motor_set_right_level(0, 1);
        }

        if (direction_changed) {
            direction_changed = false;

            if (direction == left) {
                motor_set_right_level(level_right, 0);
                motor_set_left_level(level_left, 1);
            }
            else if (direction == right) {
                motor_set_right_level(level_right, 1);
                motor_set_left_level(level_left, 0);
            }
            else if (direction == forward) {
                motor_set_right_level(level_right, 1);
                motor_set_left_level(level_left, 1);
            }
            else if (direction == backward) {
                motor_set_right_level(level_right, 0);
                motor_set_left_level(level_left, 0);
            }
            else if (direction == start) {
                motor_set_right_level(level_right, 1);
                motor_set_left_level(level_left, 1);
            }
             else if (direction == stop) {
                motor_set_right_level(0, 1);
                motor_set_left_level(0, 1);
            }
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
