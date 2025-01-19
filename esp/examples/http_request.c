#include "esp_http_client.h"

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // Log the data as a string
            ESP_LOGI(HTTP_TAG, "Response: %.*s", evt->data_len, (char *)evt->data);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
    {
        char redirect_url[256]; // Adjust size if necessary
        if (esp_http_client_get_url(evt->client, redirect_url, sizeof(redirect_url)) == ESP_OK)
        {
            ESP_LOGI(HTTP_TAG, "HTTP_EVENT_REDIRECT: Redirecting to %s", redirect_url);
        }
        else
        {
            ESP_LOGE(HTTP_TAG, "HTTP_EVENT_REDIRECT: Failed to get redirect URL");
        }
        break;
    }
    default:
        ESP_LOGW(HTTP_TAG, "Unknown event: %d", evt->event_id);
        break;
    }
    return ESP_OK;
}

void https_request(void)
{
    esp_http_client_config_t config = {
        .url = SERVER_ENDPOINT,
        .cert_pem = (const char *)isrg_root_x1_pem,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(HTTP_TAG, "HTTPS Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(HTTP_TAG, "HTTPS Request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void on_wifi_connected()
{
    ESP_LOGI(HTTP_TAG, "Connected to Wi-Fi. Waiting to stabilize...");
    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 seconds delay
    ESP_LOGI(HTTP_TAG, "Starting initial HTTP request...");
    https_request();
}
