#include <stdio.h>
#include "esp_log.h"

static const char *TAG_SYS = "SYSTEM";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG_SYS, "BCA152 FreeRTOS Multisensor");
    ESP_LOGI(TAG_SYS, "System starting...");
}