#include "wifi_connect.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    connect_to_wifi();

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
