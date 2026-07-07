#include "heart_rate.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"

#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

static const char *TAG = "HEART_RATE";

static adc_oneshot_unit_handle_t g_adc_handle = nullptr;

static const size_t HR_BUFFER_SIZE = 800;
static int g_samples[HR_BUFFER_SIZE];
static size_t g_sampleCount = 0;
static uint32_t g_windowStart = 0;
static bool g_alertActive = false;
static float g_smoothedBpm = 0.0f;

static int readPulseSample() {
    if (!g_adc_handle) {
        return 0;
    }
    int raw_val = 0;
    esp_err_t err = adc_oneshot_read(g_adc_handle, ADC_CHANNEL_6, &raw_val);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Falha na leitura do ADC: %s", esp_err_to_name(err));
        return 0;
    }
    return raw_val;
}

static float computeBpmFromPeaks() {
    if (g_sampleCount < 50) {
        return 0.0f;
    }

    long sum = 0;
    for (size_t i = 0; i < g_sampleCount; ++i) {
        sum += g_samples[i];
    }
    const int baseline = static_cast<int>(sum / static_cast<long>(g_sampleCount));

    uint32_t lastPeakMs = 0;
    uint32_t intervalSum = 0;
    uint8_t peakCount = 0;
    bool rising = false;
    int prev = g_samples[0];

    for (size_t i = 1; i < g_sampleCount; ++i) {
        const int current = g_samples[i];
        const uint32_t sampleTime = g_windowStart + (i * HR_SAMPLE_INTERVAL_MS);

        if (!rising && prev <= baseline && current > baseline + 30) {
            rising = true;
        }

        if (rising && prev > baseline + 30 && current <= baseline + 20) {
            if (lastPeakMs > 0) {
                const uint32_t interval = sampleTime - lastPeakMs;
                if (interval >= HR_MIN_PEAK_INTERVAL_MS && interval <= HR_MAX_PEAK_INTERVAL_MS) {
                    intervalSum += interval;
                    ++peakCount;
                }
            }
            lastPeakMs = sampleTime;
            rising = false;
        }

        prev = current;
    }

    if (peakCount == 0) {
        return 0.0f;
    }

    const float avgInterval = static_cast<float>(intervalSum) / peakCount;
    return 60000.0f / avgInterval;
}

void heartRateInit() {
    // Configura o ADC1 para ler o canal correspondente ao GPIO 34 (ADC1_CH6)
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = (adc_oneshot_clk_src_t)0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &g_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar unidade ADC1: %s", esp_err_to_name(err));
        return;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    err = adc_oneshot_config_channel(g_adc_handle, ADC_CHANNEL_6, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar canal ADC1_CH6: %s", esp_err_to_name(err));
        return;
    }

    g_windowStart = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "Driver ADC1 inicializado com sucesso no canal 6 (GPIO 34)");
}

void heartRateUpdate() {
    static uint32_t lastSample = 0;
    const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (now - lastSample < HR_SAMPLE_INTERVAL_MS) {
        return;
    }
    lastSample = now;

    if (g_sampleCount < HR_BUFFER_SIZE) {
        g_samples[g_sampleCount++] = readPulseSample();
    }

    if (now - g_windowStart < HR_WINDOW_MS) {
        return;
    }

    const float rawBpm = computeBpmFromPeaks();
    const bool valid = rawBpm >= HR_LOW_BPM && rawBpm <= 200.0f;

    if (valid) {
        if (g_smoothedBpm <= 0.0f) {
            g_smoothedBpm = rawBpm;
        } else {
            g_smoothedBpm = (g_smoothedBpm * 0.7f) + (rawBpm * 0.3f);
        }
    }

    bool outOfRange = valid && (g_smoothedBpm < HR_LOW_BPM || g_smoothedBpm > HR_HIGH_BPM);
    if (outOfRange) {
        if (!g_alertActive) {
            g_alertActive = true;
            buzzerPatternHrAlert();
            appStateRequestPublish();
        }
    } else {
        g_alertActive = false;
    }

    appStateSetHeartRate(g_smoothedBpm, valid, g_alertActive);

    // Log periodico de diagnostico a cada janela
    if (valid) {
        ESP_LOGI(TAG, "BPM Calculado: %.1f | Suavizado: %.1f | Alerta: %s", rawBpm, g_smoothedBpm, g_alertActive ? "ATIVO" : "INATIVO");
    } else {
        ESP_LOGD(TAG, "Leitura descartada ou ausencia de sinal (BPM invalido)");
    }

    g_sampleCount = 0;
    g_windowStart = now;
}
