#include <stdio.h>
#include "../include/utils.h"
#include "freertos/ringbuf.h"
#include "../include/motor_rpm.h"
#include "../../include/esp_now_tool.h"

#define ESP_NOW_MEMORY   4096
#define ESP_NOW_PRIORITY 2

#define MOTOR_RPM_STACK_MEMORY  2048
#define MOTOR_RPM_PRIORITY      1

#define TAG "main"
#ifdef CONFIG_ESPNOW_MASTER_MODE 
TaskHandle_t masterESPNOW;
TaskHandle_t motorRPM;

RingbufHandle_t buf_motorRPM;
#endif
#ifdef CONFIG_ESPNOW_SLAVE_MODE
TaskHandle_t slaveESPNOW;
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "Starting the system...");
#ifdef CONFIG_ESPNOW_MASTER_MODE 
    buf_motorRPM = xRingbufferCreate(1026, RINGBUF_TYPE_NOSPLIT);
    xTaskCreatePinnedToCore(motor_rpm_init, "init motor rpm", MOTOR_RPM_STACK_MEMORY, &buf_motorRPM, MOTOR_RPM_PRIORITY, &motorRPM, 1);
    xTaskCreatePinnedToCore(esp_now_master_init, "init master espnow", ESP_NOW_MEMORY, &buf_motorRPM, ESP_NOW_PRIORITY, &masterESPNOW, 0);
#endif
#ifdef CONFIG_ESPNOW_SLAVE_MODE 
    xTaskCreatePinnedToCore(esp_now_slave_init, "init slave espnow", ESP_NOW_MEMORY, NULL, ESP_NOW_PRIORITY, &slaveESPNOW, 0);
#endif
    vTaskDelay(portMAX_DELAY);
}
