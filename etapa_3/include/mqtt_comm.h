#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include "lwip/apps/mqtt.h" // Include this to get u8_t type

// Define the function signature for our single "router" callback
typedef void (*mqtt_router_callback_t)(const char *topic, const u8_t *data, u16_t len);

// Function to connect to the broker and set the main callback
void mqtt_connect_and_set_router(const char *client_id, const char *broker_ip, const char *user, const char *pass, mqtt_router_callback_t router_cb);

// Function to subscribe to a topic after connecting
void mqtt_subscribe_to_topic(const char *topic);

// Publish function (remains the same)
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);

#endif