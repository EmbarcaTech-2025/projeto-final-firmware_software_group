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
#include "hardware/uart.h"

// driver for the motor (h bridge tb6612fng)
extern "C" {
    #include "include/motor.h"
    //#include "include/mpu6050_i2c.h"
    #include "include/mpu9250_i2c.h"
    #include "include/mqtt_comm.h"
    #include "include/wifi_conn.h"
    #include "include/xor_cipher.h"
}

// driver for the TinyGPS
#include "include/TinyGPSPlus.h"

const char* mqtt_topic_location = "bitdoglab/daltro_phone/location"; 
const char* mqtt_topic_bitdoglab = "bitdoglab"; 

TinyGPSPlus gps; // instance of the TinyGPS++ object

// Global variables for suitcase's own GPS location
double suitcase_lat = 0.0;
double suitcase_lon = 0.0;

// Global variables to be updated by MQTT with the person's location
volatile double person_lat = 0.0;
volatile double person_lon = 0.0;

volatile bool is_navigating = false;
volatile bool is_manual = false;

typedef uint8_t byte;

#define RED_LED 13
#define GREEN_LED 11
#define BLUE_LED 12

#define LEFT_BUTTON 6
#define RIGHT_BUTTON 5

#define UART_RX_PIN 17
#define UART_TX_PIN 16
#define BAUD_RATE 9600
#define UART_ID uart0

char payload_buffer[256];

bool start_motor = false, stop_motor = false;

// Define the I2C port and pins at the top of your file
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1

void setup_compass() {
    i2c_init(I2C_PORT, 400 * 1000); // Initialize I2C at 400kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); // Assumes SDA on GPIO 0
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C); // Assumes SCL on GPIO 1
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Reset the MPU9250 and initialize the magnetometer
    mpu9250_reset(i2c0);
    mpu9250_init_mag(i2c0);

    printf("MPU9250 Compass Initialized.\n");
}

float read_compass_heading() {
    int16_t mag_raw[3];
    mpu9250_read_mag_raw(i2c0, mag_raw);

    // --- PLACEHOLDER CALIBRATION VALUES ---
    // You MUST replace these with your own calibration data!
    float mag_x_offset = 15.5; 
    float mag_y_offset = 52.0;

    // Apply calibration offset
    float mx = (float)mag_raw[0] - mag_x_offset;
    float my = (float)mag_raw[1] - mag_y_offset;
    
    // Calculate the heading in radians
    float heading_rad = atan2(my, mx);

    // Convert radians to degrees
    float heading_deg = heading_rad * 180.0 / M_PI;

    // --- MAGNETIC DECLINATION ---
    // This is for Brasília. Find the value for your location.
    float declination_angle = -21.4;
    heading_deg += declination_angle;

    // Normalize to 0-360 degrees
    if (heading_deg < 0) {
        heading_deg += 360;
    }

    return heading_deg;
}

void set_leds(uint red_signal, uint green_signal, uint blue_signal) {
    gpio_put(RED_LED, red_signal);
    gpio_put(GREEN_LED, green_signal);
    gpio_put(BLUE_LED, blue_signal);

}


// Reads from UART and feeds the GPS library
void read_suitcase_gps_task() {
    /*
    while (uart_is_readable(UART_ID)) {
        gps.encode(uart_getc(UART_ID));
    }
    */

    while (uart_is_readable(UART_ID)) {
        char ch = uart_getc(UART_ID);
        // --- TEMPORARY DEBUG LINE ---
        // Print every character received from the GPS
        printf("%c", ch);
        
        // Feed the character to the library
        gps.encode(ch);
    }

    // Update global variables if the location has changed
    if (gps.location.isUpdated()) {
        suitcase_lat = gps.location.lat();
        suitcase_lon = gps.location.lng();
    }
}

// Replace your old follow_me_logic() with this new function
void navigate_to_destination() {
    const double STOPPING_DISTANCE = 1.0; // Stop within 1.0 meters

    // Safety check: Don't move if our own GPS isn't ready
    if (!gps.location.isValid()) {
        printf("Waiting for own GPS fix before starting journey...\n");
        return;
    }

    // Calculate distance and bearing to the fixed destination
    double distance_to_dest = TinyGPSPlus::distanceBetween(suitcase_lat, suitcase_lon, person_lat, person_lon);
    double target_bearing = TinyGPSPlus::courseTo(suitcase_lat, suitcase_lon, person_lat, person_lon);
    float current_heading = read_compass_heading();
    float heading_error = target_bearing - current_heading;

    if (heading_error > 180)  heading_error -= 360;
    if (heading_error < -180) heading_error += 360;

    // --- Have we arrived? ---
    if (distance_to_dest <= STOPPING_DISTANCE) {
        printf("Destination reached! Stopping.\n");
        motor_set_left_level(0, 1); // Stop
        motor_set_right_level(0, 1);
        is_navigating = false; // CRITICAL: This stops the process
        set_leds(1, 0, 0);     // Set light to red (STOP)
        return;
    }

    // --- If we haven't arrived, continue navigating ---
    uint16_t turn_speed = 16384; 
    uint16_t move_speed = 24576; 
    
    if (abs(heading_error) > 25.0) {
        if (heading_error > 0) {
            motor_set_left_level(turn_speed, 1); // Turn Right
            motor_set_right_level(turn_speed, 0);
        } else {
            motor_set_left_level(turn_speed, 0); // Turn Left
            motor_set_right_level(turn_speed, 1);
        }
    } else {
        motor_set_left_level(move_speed, 1); // Move forward
        motor_set_right_level(move_speed, 1);
    }
}

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



// This function manually parses only the lat/lon from a JSON string
void parse_gps_coordinates(const char *payload) {
    // Find the pointer to the "latitude" key in the payload string
    const char *lat_ptr = strstr(payload, "\"latitude\":");
    if (lat_ptr) {
        // Read the double value that comes after the key
        sscanf(lat_ptr, "\"latitude\":%lf", &person_lat);
    }

    // Find the pointer to the "longitude" key
    const char *lon_ptr = strstr(payload, "\"longitude\":");
    if (lon_ptr) {
        // Read the double value that comes after the key
        sscanf(lon_ptr, "\"longitude\":%lf", &person_lon);
    }
    
    // Optional: Print the values to confirm they were parsed correctly
    printf("Parsed GPS: Latitude=%.8lf, Longitude=%.8lf\n", person_lat, person_lon);
}



// --- Create these two NEW handler functions ---
void handle_command_message(const char* payload) {
    // All your logic for LEDs and manual motor control goes here

    // signal the leds
    uint red_led=0, green_led=0, blue_led=0;
    if (strcmp(payload, "red_on")==0) {
        red_led = 1;
    } 
    else if (strcmp(payload, "red_off")==0) {
        red_led = 0;
    } 
    else if (strcmp(payload, "green_on")==0) {
        green_led = 1;
    } 
    else if (strcmp(payload, "green_off")==0) {
        green_led = 0;
    } 
    else if (strcmp(payload, "blue_on")==0) {
        blue_led = 1;
    } 
    else if (strcmp(payload, "blue_off")==0) {
        blue_led = 0;
    } 
    set_leds(red_led, green_led, blue_led);

    // control the start or stop of the motors
    if (strcmp(payload, "start_motor") == 0 ) {
        start_motor = true;
    }

    else if (strcmp(payload, "stop_motor") == 0) {
        stop_motor = true;
    }


    // control the direction of the motors
    if (strcmp(payload, "direction_left") == 0) {
        direction_changed = true;
        direction = left;
    }
    else if (strcmp(payload, "direction_right") == 0) {
        direction_changed = true;
        direction = right;
    }
    else if (strcmp(payload, "direction_forward") == 0) {
        direction_changed = true;
        direction = forward;
    }
    else if (strcmp(payload, "direction_backward") == 0) {
        direction_changed = true;
        direction = backward;
    }
    else if (strcmp(payload, "direction_start") == 0) {
        direction_changed = true;
        direction = start;
    }
    else if (strcmp(payload, "direction_stop") == 0) {
        direction_changed = true;
        direction = stop;
    }
}


// --- Create the NEW single "router" callback ---
void mqtt_router_callback(const char *topic, const u8_t *data, u16_t len) {
    // Copy payload to a null-terminated buffer
    memset(payload_buffer, 0, sizeof(payload_buffer));
    char payload_buffer[len + 1];
    memcpy(payload_buffer, data, len);
    payload_buffer[len] = '\0';

    // Route the message to the correct handler based on the topic
    if (strcmp(topic, mqtt_topic_location) == 0) {
        parse_gps_coordinates(payload_buffer);

        // If parsing was successful, start the navigation process
        if (person_lat != 0.0 || person_lon != 0.0) {
            printf("New destination set! Starting navigation.\n");
            is_navigating = true; // This is the trigger!
            is_manual = false;
        }

    } else if (strcmp(topic, mqtt_topic_bitdoglab) == 0) {
        handle_command_message(payload_buffer);
        is_manual = true;
        is_navigating = false;
    }
}



int main() {
    // variaveis de inicializacao
    uint is_subscriber = 1;
    uint is_publisher = 0;

    uint xor_key = 42;
    const char *IP = "192.168.15.4";
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

    // setup the gps using uart
    uart_setup();

    // setup the buttons
    button_setup();

    // setup the leds
    leds_setup();
    
    // setup and enable motor
    motor_setup();
    motor_enable();

    // setup and enable the mpu6050
    // mpu6050_setup_i2c();
    // mpu6050_reset();
    setup_compass();

    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID)R e senha
    connect_to_wifi(SSID, SSID_PASSWORD);

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    if (is_subscriber) {
        strcpy(client_id, client_subscriber);
        
        // 1. Connect ONCE and set the router callback
        mqtt_connect_and_set_router(client_id, IP, USER, USER_PASSWORD, mqtt_router_callback);

        // Wait a moment for the connection to establish before subscribing
        sleep_ms(2000); 

        // 2. Subscribe to all your topics
        mqtt_subscribe_to_topic(mqtt_topic_bitdoglab);
        mqtt_subscribe_to_topic(mqtt_topic_location);
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
        //sleep_ms(delay_time_ms);
        
        
        /*

        // control for the direction using the accelerometer/gyroscope
        mpu6050_read_raw(accel_raw, gyro_raw, &temp);

        // Convert to g's and degrees per second
        for (uint8_t i = 0; i < 3; i++) {
            accel[i] = (float)accel_raw[i] / 16384.0f;
            gyro[i] = (float)gyro_raw[i] / 131.0f;
            //angles[i] = (180*asinf(accel[i]))/M_PI;
        }


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



        // acumulated_delay_time_ms+=delay_time_ms;
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

        /*
        // read the values from the magnetometer
         int16_t mag_raw[3]; // X, Y, Z
        mpu9250_read_mag_raw(I2C_PORT, mag_raw);

        // Print the raw values from the magnetometer
        printf("Raw Mag -> X: %d, Y: %d, Z: %d\n", mag_raw[0], mag_raw[1], mag_raw[2]);
        
        //sleep_ms(250);
        */

        // --- 1. READ ALL SENSORS ---
        read_suitcase_gps_task(); // Update suitcase's own location from its GPS

        // --- 2. CHECK FOR MANUAL COMMANDS ---
        // Check for manual start/stop commands from buttons or MQTT
        if (!gpio_get(RIGHT_BUTTON) || start_motor) {
            start_motor = true;
            stop_motor = false;
        }
        if (!gpio_get(LEFT_BUTTON) || stop_motor) {
            start_motor = false;
            stop_motor = true;
        }

        
        // Check if we are currently in the process of navigating
        if (is_navigating) {
            set_leds(0, 1, 0); // Green light for "GO"
            navigate_to_destination();
        } else if (!is_manual) {
            // If not navigating, do nothing and make sure motors are off and light is red
            set_leds(1, 0, 0); // Red light for "IDLE/STOP"
            motor_set_left_level(0, 1);
            motor_set_right_level(0, 1);
        }

        sleep_ms(500); // Main loop delay
        
    }

    return 0;
}
