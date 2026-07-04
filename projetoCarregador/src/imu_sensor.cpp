#include "imu_sensor.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

static const char *TAG = "IMU_SENSOR";

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TIMEOUT_MS 1000

static bool g_imu_initialized = false;

static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mpu6050_register_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    if (len == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void imuInit() {
    // 1. Inicializa o barramento I2C se ainda nao foi feito
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = static_cast<gpio_num_t>(PIN_I2C_SDA); // 21
    conf.scl_io_num = static_cast<gpio_num_t>(PIN_I2C_SCL); // 22
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    conf.clk_flags = 0;

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err == ESP_OK) {
        esp_err_t install_err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
        if (install_err == ESP_OK) {
            ESP_LOGI(TAG, "I2C master driver instalado no canal %d (SDA: %d, SCL: %d)", I2C_MASTER_NUM, PIN_I2C_SDA, PIN_I2C_SCL);
        } else if (install_err == ESP_ERR_INVALID_STATE) {
            ESP_LOGI(TAG, "I2C master driver ja estava instalado no canal %d", I2C_MASTER_NUM);
        } else {
            ESP_LOGE(TAG, "Erro ao instalar I2C master: %s", esp_err_to_name(install_err));
            return;
        }
    } else {
        ESP_LOGE(TAG, "Erro ao configurar parametros I2C: %s", esp_err_to_name(err));
        return;
    }

    // 2. Acorda o MPU-6050 escrevendo 0 no registrador PWR_MGMT_1 (0x6B)
    err = mpu6050_register_write_byte(0x6B, 0x00);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MPU-6050 configurado com sucesso e acordado.");
        g_imu_initialized = true;
    } else {
        ESP_LOGE(TAG, "Falha ao se comunicar com MPU-6050 no endereco I2C 0x%02X: %s", IMU_I2C_ADDR, esp_err_to_name(err));
        g_imu_initialized = false;
    }
}

void imuUpdate() {
    if (!g_imu_initialized) {
        return;
    }

    static uint32_t lastSample = 0;
    const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (now - lastSample < ACCEL_SAMPLE_INTERVAL_MS) {
        return;
    }
    lastSample = now;

    // Leitura dos registradores 0x3B (ACCEL_XOUT_H) ate 0x40 (ACCEL_ZOUT_L)
    uint8_t data[6] = {0};
    esp_err_t err = mpu6050_register_read(0x3B, data, 6);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Falha ao ler dados brutos da IMU: %s", esp_err_to_name(err));
        return;
    }

    // Combina os bytes em inteiros de 16 bits sinalizados
    int16_t raw_x = (data[0] << 8) | data[1];
    int16_t raw_y = (data[2] << 8) | data[3];
    int16_t raw_z = (data[4] << 8) | data[5];

    // Escala padrao do MPU-6050 e de +/- 2g (sensibilidade de 16384 LSB/g)
    float ax = raw_x / 16384.0f;
    float ay = raw_y / 16384.0f;
    float az = raw_z / 16384.0f;

    // Magnitude do vetor aceleracao (em G)
    float magnitude = sqrtf(ax * ax + ay * ay + az * az);

    // Deteccao de movimento brusco
    static uint32_t lastAlertTime = 0;
    bool sudden_mvmt = magnitude > ACCEL_LIMIT_G;

    if (sudden_mvmt) {
        if (now - lastAlertTime >= ACCEL_COOLDOWN_MS) {
            lastAlertTime = now;
            ESP_LOGW(TAG, "Movimento brusco detectado! Magnitude: %.2f G (Limite: %.2f G)", magnitude, ACCEL_LIMIT_G);
            
            // Aciona o buzzer local
            buzzerPatternMotion();
            
            // Solicita envio imediato por telemetria MQTT
            appStateRequestPublish();
        }
    }

    // Atualiza o estado global
    appStateSetAccel(magnitude, sudden_mvmt);
}
