#include <stdio.h>
#include "../include/utils.h"
#include "../include/motor_rpm.h"
#include "../include/esp_now_master.h"

#define MASTER_ESP_NOW_MEMORY   8192
#define MASTER_ESP_NOW_PRIORITY 1

#define MOTOR_RPM_STACK_MEMORY  2048
#define MOTOR_RPM_PRIORITY      2

#define TAG "main"

TaskHandle_t masterESPNOW;
TaskHandle_t motorRPM;

void app_main(void)
{
    ESP_LOGI(TAG, "Starting the system...");
    xTaskCreatePinnedToCore(motor_rpm_init, "init motor rpm", MOTOR_RPM_STACK_MEMORY, NULL, MOTOR_RPM_PRIORITY, &motorRPM, 1);
    xTaskCreatePinnedToCore(esp_now_master_init, "init master espnow", MASTER_ESP_NOW_MEMORY, NULL, MASTER_ESP_NOW_PRIORITY, &masterESPNOW, 0);
    vTaskDelay(portMAX_DELAY);
}
