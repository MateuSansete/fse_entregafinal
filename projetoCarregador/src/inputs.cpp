#include "inputs.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "INPUTS";

static volatile int g_encoder_delta = 0;
static portMUX_TYPE g_encoder_mux = SPINLOCK_INITIALIZER;

// Controle de tempo de pressao do botao SOS/Nav (GPIO 4)
static uint32_t g_btn_pressed_ms = 0;
static bool g_sos_triggered_this_press = false;

static void IRAM_ATTR encoder_isr_handler(void* arg) {
    (void)arg;
    static uint32_t last_time = 0;
    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    // Debounce de 50 ms na interrupcao do encoder
    if (now - last_time > 50) {
        int dt_level = gpio_get_level(static_cast<gpio_num_t>(PIN_ENC_DT));
        
        portENTER_CRITICAL_ISR(&g_encoder_mux);
        if (dt_level == 1) {
            g_encoder_delta = g_encoder_delta + 1;
        } else {
            g_encoder_delta = g_encoder_delta - 1;
        }
        portEXIT_CRITICAL_ISR(&g_encoder_mux);
        
        last_time = now;
    }
}

void inputsInit() {
    // 1. Configura os pinos CLK e DT do encoder rotativo
    gpio_config_t enc_conf = {};
    enc_conf.pin_bit_mask = (1ULL << PIN_ENC_CLK) | (1ULL << PIN_ENC_DT);
    enc_conf.mode = GPIO_MODE_INPUT;
    enc_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    enc_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    enc_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&enc_conf);

    // Configura CLK com interrupcao na borda de descida
    gpio_set_intr_type(static_cast<gpio_num_t>(PIN_ENC_CLK), GPIO_INTR_NEGEDGE);

    // 2. Configura os pinos de botao (NAV: GPIO 4, SW: GPIO 23)
    gpio_config_t btn_conf = {};
    btn_conf.pin_bit_mask = (1ULL << PIN_BTN_NAV) | (1ULL << PIN_ENC_SW);
    btn_conf.mode = GPIO_MODE_INPUT;
    btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    btn_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    btn_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&btn_conf);

    // 3. Instala o servico de interrupcao GPIO (ignora se ja instalado)
    esp_err_t err = gpio_install_isr_service(0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Servico ISR do GPIO instalado com sucesso.");
    } else if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Servico ISR do GPIO ja estava instalado.");
    } else {
        ESP_LOGE(TAG, "Erro ao instalar servico ISR do GPIO: %s", esp_err_to_name(err));
        return;
    }

    // 4. Registra o ISR handler para o pino CLK do encoder
    err = gpio_isr_handler_add(static_cast<gpio_num_t>(PIN_ENC_CLK), encoder_isr_handler, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao registrar ISR handler para CLK: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Inputs (Encoder e Botoes) inicializados com sucesso.");
}

void inputsUpdate() {
    // --- 1. Processamento da Rotacao do Encoder ---
    int delta = 0;
    portENTER_CRITICAL(&g_encoder_mux);
    delta = g_encoder_delta;
    g_encoder_delta = 0;
    portEXIT_CRITICAL(&g_encoder_mux);

    if (delta != 0) {
        AppState state = appStateSnapshot();
        int8_t current = static_cast<int8_t>(state.currentScreen);
        
        current = (current + delta) % SCREEN_COUNT;
        if (current < 0) {
            current += SCREEN_COUNT;
        }
        
        ScreenId next_screen = static_cast<ScreenId>(current);
        appStateSetScreen(next_screen);
        
        ESP_LOGI(TAG, "Encoder rotacionado. Tela alterada para: %s (delta: %d)", 
                 screenName(next_screen), delta);
    }

    // --- 2. Processamento do Botao SOS/Nav (GPIO 4) ---
    // Botao pressionado em nivel logico baixo (Active Low devido ao pull-up)
    int btn_level = gpio_get_level(static_cast<gpio_num_t>(PIN_BTN_NAV));
    
    if (btn_level == 0) {
        // Incrementa o tempo pressionado baseado na taxa de execucao (100 ms)
        g_btn_pressed_ms = g_btn_pressed_ms + 100;
        
        // Se ultrapassou o limiar de pressao longa para SOS
        if (g_btn_pressed_ms >= SOS_LONG_PRESS_MS && !g_sos_triggered_this_press) {
            g_sos_triggered_this_press = true;
            appStateTriggerSos(true); // Ativa SOS manual no estado
            buzzerPatternSos();       // Dispara som de SOS local
            ESP_LOGW(TAG, "Botao SOS pressionado por longo periodo (>= %d ms)! SOS disparado.", (int)SOS_LONG_PRESS_MS);
        }
    } else {
        // Se o botao foi solto, verifica se foi um clique curto valido (respeitando debounce)
        if (g_btn_pressed_ms >= BTN_DEBOUNCE_MS && g_btn_pressed_ms < SOS_LONG_PRESS_MS) {
            AppState state = appStateSnapshot();
            int8_t current = static_cast<int8_t>(state.currentScreen);
            
            // Navega ciclicamente para a proxima tela
            current = (current + 1) % SCREEN_COUNT;
            ScreenId next_screen = static_cast<ScreenId>(current);
            appStateSetScreen(next_screen);
            
            ESP_LOGI(TAG, "Clique curto detectado (%d ms). Tela alterada para: %s", 
                     (int)g_btn_pressed_ms, screenName(next_screen));
        }
        
        // Reseta o estado de leitura do botao
        g_btn_pressed_ms = 0;
        g_sos_triggered_this_press = false;
    }
}
