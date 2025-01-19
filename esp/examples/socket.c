#include "esp_log.h"
#include "lwip/sockets.h"

void test_socket_connection()
{
    const char *broker_host = "url_or_ip_address";
    int broker_port = "some_port";

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
