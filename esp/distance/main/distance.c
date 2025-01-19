#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include "freertos/event_groups.h"
#include <inttypes.h>
#include "wifi_credentials.h"
#include "fly_io_ca_pem.h"

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

void initialize_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    while (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(WIFI_TAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void on_wifi_connected()
{
    ESP_LOGI(WIFI_TAG, "Connected to Wi-Fi. Waiting to stabilize...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 seconds delay
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    ESP_LOGI(MQTT_TAG, "Event dispatched. Event ID: %d", (int)event_id);

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(event->client, "esp32/trigger", 0);
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
    ESP_LOGI(MQTT_TAG, "MQTT_BROKER_URI: %s", MQTT_BROKER_URI);

    // Correctly initialize the config with nested braces
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI, // Set the broker URI
            },
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

#include "esp_log.h"
#include "lwip/sockets.h"

void test_socket_connection()
{
    const char *broker_host = "66.241.125.108";
    // const char *broker_host = "iot-white-pond-1937.fly.dev";
    int broker_port = 1883;

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(broker_port);

    inet_pton(AF_INET, broker_host, &dest_addr.sin_addr);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0)
    {
        ESP_LOGE("SOCKET_TEST", "Socket creation failed: errno %d", errno);
        return;
    }

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE("SOCKET_TEST", "Socket connection failed: errno %d", errno);
    }
    else
    {
        ESP_LOGI("SOCKET_TEST", "Successfully connected to %s:%d", broker_host, broker_port);
    }

    close(sock);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    };

    wifi_init();

    ESP_LOGI(WIFI_TAG, "Waiting for connection...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for Wi-Fi connection

    initialize_sntp();
    on_wifi_connected();

    test_socket_connection();

    mqtt_init();
}
