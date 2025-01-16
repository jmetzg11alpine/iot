#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "wifi_config.h"

static const char *WIFI_TAG = "WiFi";
static const char *HTTP_CLIENT_TAG = "HTTP_CLIENT";

// URL of your Gin server
#define SERVER_URL "http://192.168.1.248:8080/connected"

// Event handler for Wi-Fi and IP events
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(WIFI_TAG, "Wi-Fi started, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(WIFI_TAG, "Disconnected from Wi-Fi, retrying...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        esp_ip4_addr_t *ip = &((ip_event_got_ip_t *)event_data)->ip_info.ip;
        ESP_LOGI(WIFI_TAG, "Got IP Address: " IPSTR, IP2STR(ip));
    }
}

void connect_to_wifi(void)
{
    // Initialize NVS (Non-Volatile Storage) for Wi-Fi settings
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi station
    esp_netif_create_default_wifi_sta();

    // Configure Wi-Fi with SSID and password
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Set Wi-Fi mode to station and apply configuration
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "Wi-Fi initialization complete.");
}

void send_message_to_server(const char *message)
{
    esp_http_client_config_t config = {
        .url = SERVER_URL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the POST data
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, message, strlen(message));

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        char buffer[256]; // Adjust size based on expected response
        int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
        if (len > 0)
        {
            buffer[len] = '\0'; // Null-terminate the response
            ESP_LOGI(HTTP_CLIENT_TAG, "HTTP POST Status = %d, Response = %s",
                     esp_http_client_get_status_code(client), buffer);
        }
        else
        {
            ESP_LOGI(HTTP_CLIENT_TAG, "HTTP POST Status = %d, No response body",
                     esp_http_client_get_status_code(client));
        }
    }
    else
    {
        ESP_LOGE(HTTP_CLIENT_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    // Clean up
    esp_http_client_cleanup(client);
}

void app_main(void)
{
    // Connect to Wi-Fi
    connect_to_wifi();
    printf("I connected !!!!\n");

    const char *message = "{\"status\":\"connected\"}";
    send_message_to_server(message);

    // Main loop can handle additional logic
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
