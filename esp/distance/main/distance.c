#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <inttypes.h>
#include "wifi_credentials.h"
#include "ca_pem.h"
#include "esp_timer.h"

static const char *WIFI_TAG = "WIFI";
static const char *MQTT_TAG = "MQTT";

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(WIFI_TAG, "Disconnected from Wi-Fi. Attempting to reconnect...");
        esp_wifi_connect();
    }
}

void wifi_init(void)
{
    ESP_LOGI(WIFI_TAG, "Initializing Wi-Fi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    esp_wifi_connect();
}

void on_wifi_connected()
{
    ESP_LOGI(WIFI_TAG, "Connected to Wi-Fi. Waiting to stabilize...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 seconds delay
}

#define TRIG_PIN GPIO_NUM_32 // Pin labeled D15
#define ECHO_PIN GPIO_NUM_33 // Pin labeled D14
#define TIMEOUT_US 30000     // Timeout in microseconds

// Function to measure distance using the HC-SR04
float measure_distance()
{
    // Send a 20us HIGH pulse on TRIG_PIN
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(20); // Ensure pulse is long enough
    gpio_set_level(TRIG_PIN, 0);

    // Wait for the ECHO_PIN to go HIGH
    int64_t start_time = 0, end_time = 0;
    int64_t wait_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0)
    {
        if (esp_timer_get_time() - wait_start > TIMEOUT_US)
        {
            return -1.0; // Timeout
        }
    }
    start_time = esp_timer_get_time();

    // Wait for the ECHO_PIN to go LOW
    wait_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1)
    {
        if (esp_timer_get_time() - wait_start > TIMEOUT_US)
        {
            return -1.0; // Timeout
        }
    }
    end_time = esp_timer_get_time();

    // Calculate the duration in microseconds
    int64_t duration_us = end_time - start_time;

    // Calculate and return distance in cm
    if (duration_us > 0 && duration_us < TIMEOUT_US)
    {
        return (duration_us / 2.0) * 0.0343;
    }
    else
    {
        return -1.0; // Invalid measurement
    }
}

char *get_distance()
{
    // Allocate memory for the result string
    char *measurements_str = malloc(256);
    if (!measurements_str)
    {
        return NULL;
    }
    measurements_str[0] = '\0'; // Initialize the string

    char buffer[16]; // Temporary buffer for each measurement

    for (int i = 0; i < 10; i++)
    {
        float distance = measure_distance();

        if (distance >= 0)
        {
            snprintf(buffer, sizeof(buffer), "%.2f", distance);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Invalid");
        }

        strcat(measurements_str, buffer);
        if (i < 9)
        {
            strcat(measurements_str, ", ");
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay between measurements
    }

    ESP_LOGI(MQTT_TAG, "Measurements: [%s]", measurements_str);
    return measurements_str;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)handler_args;

    ESP_LOGI(MQTT_TAG, "Event dispatched. Event ID: %d", (int)event_id);

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(event->client, MQTT_SUB, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(MQTT_TAG, "Topic: %.*s", event->topic_len, event->topic);
        ESP_LOGI(MQTT_TAG, "Data: %.*s", event->data_len, event->data);

        if (strncmp(event->data, "get_distance", event->data_len) == 0)
        {
            ESP_LOGI(MQTT_TAG, "Triggering get_distance() function...");

            // Call get_distance and get the result string
            char *distance_str = get_distance();

            if (distance_str)
            {
                int msg_id = esp_mqtt_client_publish(client, MQTT_PUB, distance_str, 0, 1, 0);
                if (msg_id == -1)
                {
                    ESP_LOGE(MQTT_TAG, "Failed to publish message to server/distance");
                }
                else
                {
                    ESP_LOGI(MQTT_TAG, "Published message to server/distance, msg_id=%d", msg_id);
                }
                // Free the allocated memory
                free(distance_str);
            }
            else
            {
                ESP_LOGE(MQTT_TAG, "Failed to get distance measurements");
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle)
        {
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS)
            {
                ESP_LOGE(MQTT_TAG, "ESP-TLS Error: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(MQTT_TAG, "TLS Stack Error: 0x%x", event->error_handle->esp_tls_stack_err);
            }
            else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
            {
                ESP_LOGE(MQTT_TAG, "Connection refused, return code: 0x%x", event->error_handle->connect_return_code);
            }
            else
            {
                ESP_LOGE(MQTT_TAG, "Other error type: %d", event->error_handle->error_type);
            }
        }
        break;

    default:
        ESP_LOGI(MQTT_TAG, "Unhandled event: %d", (int)event_id);
        break;
    }
}

void mqtt_init()
{
    ESP_LOGI(MQTT_TAG, "Initializing MQTT client with broker URI: %s", MQTT_BROKER_URI);

    // Correctly initialize the config with nested braces
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI,
            .verification.certificate = (const char *)ca_pem,
        },
        .network = {
            .disable_auto_reconnect = false, // Ensure auto-reconnect is enabled
        },
    };

    // Initialize the MQTT client
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
    {
        ESP_LOGE(MQTT_TAG, "Failed to initialize MQTT client");
        return;
    }

    // Register the event handler
    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    if (err != ESP_OK)
    {
        ESP_LOGE(MQTT_TAG, "Failed to register event handler: %s", esp_err_to_name(err));
        return;
    }

    // Start the MQTT client
    err = esp_mqtt_client_start(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(MQTT_TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(MQTT_TAG, "MQTT client started successfully");
    }
}

void app_main(void)
{
    // Configure TRIG_PIN as output
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);

    // Configure ECHO_PIN as input with pull-down
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ECHO_PIN, GPIO_PULLDOWN_ONLY);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    };

    wifi_init();

    ESP_LOGI(WIFI_TAG, "Waiting for connection...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for Wi-Fi connection

    on_wifi_connected();

    mqtt_init();
}
