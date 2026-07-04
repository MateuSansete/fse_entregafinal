#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "app_state.h"
#include "buzzer.h"
#include "config.h"
#include "display_ui.h"
#include "heart_rate.h"
#include "imu_sensor.h"
#include "inputs.h"
#include "network.h"

static const char *TAG = "SYS";

static void sensorTask(void *param) {
    (void)param;
    ESP_LOGI(TAG, "sensorTask iniciada");

    for (;;) {
        heartRateUpdate();
        imuUpdate();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void uiTask(void *param) {
    (void)param;
    ESP_LOGI(TAG, "uiTask iniciada");

    for (;;) {
        inputsUpdate();
        buzzerUpdate();
        displayUpdate(appStateSnapshot());
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void networkTask(void *param) {
    (void)param;
    ESP_LOGI(TAG, "networkTask iniciada");

    for (;;) {
        networkUpdate();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== Smartwatch IoT — ESP32 (ESP-IDF) ===");

    appStateInit();
    buzzerInit();

    // O barramento I2C sera inicializado no driver ou em modulo compartilhado futuramente.
    displayInit();
    imuInit();
    heartRateInit();
    inputsInit();
    networkInit();

    xTaskCreatePinnedToCore(sensorTask, "sensors", TASK_STACK_SENSORS, nullptr,
                          TASK_PRIORITY_SENSORS, nullptr, 1);
    xTaskCreatePinnedToCore(uiTask, "ui", TASK_STACK_UI, nullptr, TASK_PRIORITY_UI, nullptr, 0);
    xTaskCreatePinnedToCore(networkTask, "network", TASK_STACK_NETWORK, nullptr,
                          TASK_PRIORITY_NETWORK, nullptr, 0);

    ESP_LOGI(TAG, "Tasks de coordenacao iniciadas no FreeRTOS");
}
