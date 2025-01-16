#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_ping.h"
#include "esp_netif.h"
#include "ping/ping_sock.h"
#include "wifi_credentials.h"

static const char *TAG = "PING";

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected from Wi-Fi. Attempting to reconnect...");
        esp_wifi_connect();
    }
}

void wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

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

void on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint32_t elapsed_time;
    ip_addr_t target_addr;

    // Get the elapsed time
    if (esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(uint32_t)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get ping time.");
        return;
    }

    // Get the target address
    if (esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(ip_addr_t)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get ping target address.");
        return;
    }

    // Log the results
    ESP_LOGI(TAG, "Ping success: target_addr=" IPSTR ", time=%lu ms",
             IP2STR(&target_addr.u_addr.ip4), elapsed_time);
}

void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    ip_addr_t target_addr;

    // Get the target address
    if (esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(ip_addr_t)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get ping target address.");
        return;
    }

    // Log the timeout
    ESP_LOGW(TAG, "Ping timeout: target_addr=" IPSTR, IP2STR(&target_addr.u_addr.ip4));
}

void on_ping_end(esp_ping_handle_t hdl, void *args)
{
    ESP_LOGI(TAG, "Ping session finished.");
    esp_ping_delete_session(hdl);
}

void ping_google(void)
{
    ip_addr_t target_addr;
    IP_ADDR4(&target_addr, 8, 8, 8, 8); // Google's public DNS server

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.data_size = 32;
    ping_config.target_addr = target_addr; // Set the target address
    ping_config.count = 4;                 // Number of ping requests

    esp_ping_callbacks_t cbs = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = on_ping_end,
    };

    esp_ping_handle_t ping;
    ESP_ERROR_CHECK(esp_ping_new_session(&ping_config, &cbs, &ping));
    esp_ping_start(ping);
}

void on_wifi_connected()
{
    ESP_LOGI(TAG, "Connected to Wi-Fi. Waiting to stabilize...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 seconds delay
    ESP_LOGI(TAG, "Starting ping...");
    ping_google();
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

    ESP_LOGI(TAG, "Waiting for connection...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for Wi-Fi connection

    on_wifi_connected();
}
