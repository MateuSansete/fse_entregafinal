#include "buzzer.h"
#include "config.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "BUZZER";

#define MAX_BUZZER_STEPS 32

struct BuzzerPattern {
    uint8_t numSteps;
    uint8_t currentStep;
    uint32_t stepStartMs;
    struct {
        bool state;
        uint16_t durationMs;
    } steps[MAX_BUZZER_STEPS];
};

static BuzzerPattern g_pattern;
static portMUX_TYPE g_buzzer_mux = SPINLOCK_INITIALIZER;

static void startPattern(const BuzzerPattern &pat) {
    portENTER_CRITICAL(&g_buzzer_mux);
    g_pattern = pat;
    g_pattern.currentStep = 0;
    g_pattern.stepStartMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    gpio_set_level(static_cast<gpio_num_t>(PIN_BUZZER), g_pattern.steps[0].state ? 1 : 0);
    portEXIT_CRITICAL(&g_buzzer_mux);
}

void buzzerInit() {
    // Configura o pino do buzzer como saida digital
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIN_BUZZER);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    esp_err_t err = gpio_config(&io_conf);
    if (err == ESP_OK) {
        gpio_set_level(static_cast<gpio_num_t>(PIN_BUZZER), 0);
        ESP_LOGI(TAG, "Driver do Buzzer ativo inicializado no GPIO %d.", PIN_BUZZER);
    } else {
        ESP_LOGE(TAG, "Falha ao configurar GPIO %d do Buzzer: %s", PIN_BUZZER, esp_err_to_name(err));
    }
}

void buzzerUpdate() {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    portENTER_CRITICAL(&g_buzzer_mux);
    if (g_pattern.numSteps == 0) {
        portEXIT_CRITICAL(&g_buzzer_mux);
        return;
    }
    
    uint32_t elapsed = now - g_pattern.stepStartMs;
    if (elapsed >= g_pattern.steps[g_pattern.currentStep].durationMs) {
        g_pattern.currentStep++;
        if (g_pattern.currentStep >= g_pattern.numSteps) {
            // Fim do padrao sonoro, desliga o buzzer
            g_pattern.numSteps = 0;
            gpio_set_level(static_cast<gpio_num_t>(PIN_BUZZER), 0);
        } else {
            g_pattern.stepStartMs = now;
            gpio_set_level(static_cast<gpio_num_t>(PIN_BUZZER), g_pattern.steps[g_pattern.currentStep].state ? 1 : 0);
        }
    }
    portEXIT_CRITICAL(&g_buzzer_mux);
}

void buzzerBeep(uint16_t durationMs) {
    ESP_LOGD(TAG, "Beep simples: %d ms", durationMs);
    BuzzerPattern pat = {};
    pat.numSteps = 1;
    pat.steps[0].state = true;
    pat.steps[0].durationMs = durationMs;
    startPattern(pat);
}

void buzzerPatternHrAlert() {
    ESP_LOGI(TAG, "Executando bip duplo: Alerta de Frequencia Cardiaca");
    BuzzerPattern pat = {};
    pat.numSteps = 4;
    
    // Bip 1
    pat.steps[0].state = true;
    pat.steps[0].durationMs = 200;
    pat.steps[1].state = false;
    pat.steps[1].durationMs = 200;
    
    // Bip 2
    pat.steps[2].state = true;
    pat.steps[2].durationMs = 200;
    pat.steps[3].state = false;
    pat.steps[3].durationMs = 600;
    
    startPattern(pat);
}

void buzzerPatternMotion() {
    ESP_LOGI(TAG, "Executando padrao de 3 bips rapidos: Alerta de Movimento Brusco");
    BuzzerPattern pat = {};
    pat.numSteps = 6;
    
    pat.steps[0].state = true;
    pat.steps[0].durationMs = 100;
    pat.steps[1].state = false;
    pat.steps[1].durationMs = 100;
    
    pat.steps[2].state = true;
    pat.steps[2].durationMs = 100;
    pat.steps[3].state = false;
    pat.steps[3].durationMs = 100;
    
    pat.steps[4].state = true;
    pat.steps[4].durationMs = 100;
    pat.steps[5].state = false;
    pat.steps[5].durationMs = 500;
    
    startPattern(pat);
}

void buzzerPatternSos() {
    ESP_LOGI(TAG, "Executando sinal de alerta em codigo Morse: S.O.S");
    BuzzerPattern pat = {};
    pat.numSteps = 18;
    
    // S (...): 3 bips curtos de 200ms
    pat.steps[0] = {true, 200};
    pat.steps[1] = {false, 200};
    pat.steps[2] = {true, 200};
    pat.steps[3] = {false, 200};
    pat.steps[4] = {true, 200};
    pat.steps[5] = {false, 200};
    
    // O (---): 3 bips longos de 600ms
    pat.steps[6] = {true, 600};
    pat.steps[7] = {false, 200};
    pat.steps[8] = {true, 600};
    pat.steps[9] = {false, 200};
    pat.steps[10] = {true, 600};
    pat.steps[11] = {false, 200};
    
    // S (...): 3 bips curtos de 200ms
    pat.steps[12] = {true, 200};
    pat.steps[13] = {false, 200};
    pat.steps[14] = {true, 200};
    pat.steps[15] = {false, 200};
    pat.steps[16] = {true, 200};
    pat.steps[17] = {false, 1000};
    
    startPattern(pat);
}
