#include "lwip/apps/mqtt.h"
#include "include/mqtt_comm.h"
#include "lwipopts.h"

// --- Global variables for the driver ---
static mqtt_client_t *client;
static char current_topic[128]; // Buffer to hold the topic of the current message
static mqtt_router_callback_t global_router_callback = NULL; // Pointer to our main router function

// --- Internal LwIP Callbacks ---

// This callback is triggered FIRST, giving us the topic name
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    // Copy the topic into our global buffer
    strncpy(current_topic, topic, sizeof(current_topic) - 1);
    current_topic[sizeof(current_topic) - 1] = '\0'; // Ensure null termination
}

// This callback is triggered SECOND, giving us the message payload
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    // If our main router function is set, call it with the topic and data
    if (global_router_callback != NULL) {
        global_router_callback(current_topic, data, len);
    }
}

// This callback is triggered when the connection status changes
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao broker MQTT com sucesso!\n");
    } else {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
}

// --- Public Functions ---

void mqtt_connect_and_set_router(const char *client_id, const char *broker_ip, const char *user, const char *pass, mqtt_router_callback_t router_cb) {
    ip_addr_t broker_addr;
    
    if (!ip4addr_aton(broker_ip, &broker_addr)) {
        printf("Erro no IP\n");
        return;
    }

    client = mqtt_client_new();
    if (client == NULL) {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    // Store the main router function pointer
    global_router_callback = router_cb;

    // Set the LwIP internal callbacks
    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);

    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,
        .client_user = user,
        .client_pass = pass,
        .keep_alive = 60 // Keep alive in seconds
    };

    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb, NULL, &ci);
}

void mqtt_subscribe_to_topic(const char *topic) {
    if (client == NULL || !mqtt_client_is_connected(client)) {
        printf("Não é possível inscrever-se, cliente não conectado.\n");
        return;
    }
    
    err_t err = mqtt_subscribe(client, topic, 0, NULL, NULL);

    if (err == ERR_OK) {
        printf("Inscrito no tópico: %s\n", topic);
    } else {
        printf("Falha ao se inscrever no tópico %s, erro: %d\n", topic, err);
    }
}

// Publish function implementation (remains mostly the same)
static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result != ERR_OK) {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len) {
    if (client == NULL || !mqtt_client_is_connected(client)) {
        printf("Não é possível publicar, cliente não conectado.\n");
        return;
    }
    mqtt_publish(client, topic, data, len, 0, 0, mqtt_pub_request_cb, NULL);
}