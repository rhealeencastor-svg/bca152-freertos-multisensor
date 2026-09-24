#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void taskA(void *pvParameters) {
    for (;;) {
        printf("Task A running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskB(void *pvParameters) {
    for (;;) {
        printf("Task B running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    xTaskCreate(taskA, "TaskA", 2048, NULL, 1, NULL);
    xTaskCreate(taskB, "TaskB", 2048, NULL, 1, NULL);
}
