#include "../../include/motor_rpm.h"
#include "../../include/utils.h"
#include "driver/gpio.h"

#define TAG "motor_rpm"

#define MOTOR_RPM_PIN 14
#define MOTOR_PULSES_TO_RPM 2

SemaphoreHandle_t motor_rpm_sem;

gpio_config_t motor_rpm_gpio_config;

static void IRAM_ATTR motor_rpm_interrupt_handler(void *args)
{
    xSemaphoreGiveFromISR(motor_rpm_sem, pdFALSE);
}

static void set_gpio_motor_rpm() 
{
    gpio_config(&motor_rpm_gpio_config);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(MOTOR_RPM_PIN, motor_rpm_interrupt_handler, (void *)MOTOR_RPM_PIN);
}

void motor_rpm_init(void *p1) 
{
    ARGUNSED(p1);
    ESP_LOGI(TAG, "Starting the rpm motor thread...");
    set_gpio_motor_rpm();

    motor_rpm_sem = xSemaphoreCreateCounting(MOTOR_PULSES_TO_RPM, 0);
    if (motor_rpm_sem == NULL) {
        goto error;
    }

    while (true) {
        xSemaphoreTake(motor_rpm_sem, portMAX_DELAY);
        xSemaphoreTake(motor_rpm_sem, portMAX_DELAY);
    }

    error:
        ESP_LOGI(TAG, "Error creating semaphore");
        return;
}

gpio_config_t motor_rpm_gpio_config = {
    .pin_bit_mask = (1 << MOTOR_RPM_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE,
};