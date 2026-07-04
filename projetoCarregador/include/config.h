#pragma once

// --- Pinagem (README seção 5) ---
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define PIN_PULSE_AO 34
#define PIN_ENC_CLK 18
#define PIN_ENC_DT 19
#define PIN_ENC_SW 23
#define PIN_BTN_NAV 4
#define PIN_BUZZER 25

// --- I2C ---
#define OLED_I2C_ADDR 0x3C
#define IMU_I2C_ADDR 0x68

// --- OLED 0,98" SSD1306 ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- Frequência cardíaca ---
#define HR_LOW_BPM 50
#define HR_HIGH_BPM 120
#define HR_ALERT_PERSIST_MS 5000UL
#define HR_SAMPLE_INTERVAL_MS 5      // ~200 Hz
#define HR_WINDOW_MS 4000UL
#define HR_MIN_PEAK_INTERVAL_MS 300UL
#define HR_MAX_PEAK_INTERVAL_MS 1500UL

// --- Acelerômetro ---
#define ACCEL_LIMIT_G 2.5f
#define ACCEL_SAMPLE_INTERVAL_MS 20    // ~50 Hz
#define ACCEL_COOLDOWN_MS 5000UL

// --- SOS / navegação ---
#define SOS_LONG_PRESS_MS 3000UL
#define BTN_DEBOUNCE_MS 50UL

// --- Buzzer ---
#define BUZZER_PULSE_MS 150UL
#define BUZZER_HR_INTERVAL_MS 500UL

// --- Rede ---
#define MQTT_TELEMETRY_INTERVAL_MS 10000UL
#define WIFI_RECONNECT_INTERVAL_MS 5000UL
#define MQTT_RECONNECT_INTERVAL_MS 5000UL
#define MQTT_BUFFER_SIZE 512

// --- Tasks ---
#define TASK_STACK_SENSORS 4096
#define TASK_STACK_UI 4096
#define TASK_STACK_NETWORK 6144
#define TASK_PRIORITY_SENSORS 2
#define TASK_PRIORITY_UI 1
#define TASK_PRIORITY_NETWORK 1
