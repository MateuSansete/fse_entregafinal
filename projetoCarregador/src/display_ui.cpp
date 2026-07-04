#include "display_ui.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <time.h>

static const char *TAG = "OLED";

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TIMEOUT_MS 1000

static bool g_oled_initialized = false;
static uint8_t g_framebuffer[1024];

// Tabela de fonte 5x7 compacta para caracteres ASCII imprimíveis (32 a 126)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x4f, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7f, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // caractere de barra invertida
    {0x00, 0x41, 0x41, 0x7f, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7f, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7f}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7e, 0x09, 0x01, 0x02}, // f
    {0x0c, 0x52, 0x52, 0x52, 0x3e}, // g
    {0x7f, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7d, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3d, 0x00}, // j
    {0x7f, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7f, 0x40, 0x00}, // l
    {0x7c, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7c, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7c, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7c}, // q
    {0x7c, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3f, 0x44, 0x40, 0x20}, // t
    {0x3c, 0x40, 0x40, 0x20, 0x7c}, // u
    {0x1c, 0x20, 0x40, 0x20, 0x1c}, // v
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0c, 0x50, 0x50, 0x50, 0x3c}, // y
    {0x44, 0x64, 0x54, 0x4c, 0x44}, // z
    {0x08, 0x36, 0x41, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7f, 0x00, 0x00}, // |
    {0x00, 0x41, 0x41, 0x36, 0x08}, // }
    {0x02, 0x01, 0x02, 0x04, 0x02}  // ~
};

static void oled_clear_framebuffer() {
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
}

static void oled_draw_char(uint8_t page, uint8_t col, char c) {
    if (c < 32 || c > 126 || page >= 8 || col >= 128) {
        return;
    }
    uint8_t idx = c - 32;
    for (uint8_t i = 0; i < 5; ++i) {
        if (col + i < 128) {
            g_framebuffer[page * 128 + col + i] = font5x7[idx][i];
        }
    }
    // Deixa uma coluna de espaco apos o caractere
    if (col + 5 < 128) {
        g_framebuffer[page * 128 + col + 5] = 0x00;
    }
}

static void oled_draw_string(uint8_t page, uint8_t col, const char *str) {
    while (*str) {
        oled_draw_char(page, col, *str);
        col += 6; // Largura do caractere 5px + 1px de espaco
        str++;
    }
}

static esp_err_t oled_refresh() {
    if (!g_oled_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    // 1. Envia comando para resetar o cursor (modo de escrita horizontal completo)
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Control byte: comandos a seguir
    i2c_master_write_byte(cmd, 0x21, true); // Set Column Address
    i2c_master_write_byte(cmd, 0x00, true); // Start
    i2c_master_write_byte(cmd, 127, true);  // End
    i2c_master_write_byte(cmd, 0x22, true); // Set Page Address
    i2c_master_write_byte(cmd, 0x00, true); // Start
    i2c_master_write_byte(cmd, 7, true);    // End
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    // 2. Envia os 1024 bytes do buffer
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true); // Control byte: dados a seguir
    i2c_master_write(cmd, g_framebuffer, 1024, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void displayInit() {
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
            ESP_LOGE(TAG, "Erro ao instalar I2C master para OLED: %s", esp_err_to_name(install_err));
            return;
        }
    } else {
        ESP_LOGE(TAG, "Erro ao configurar parametros I2C: %s", esp_err_to_name(err));
        return;
    }

    // 2. Comandos de inicializacao do SSD1306 (128x64 pixels)
    static const uint8_t init_cmds[] = {
        0xAE, // Turn display off
        0xD5, 0x80, // Set display clock divide ratio
        0xA8, 0x3F, // Set multiplex ratio (64 linhas)
        0xD3, 0x00, // Set display offset (sem offset)
        0x40, // Set start line address (0)
        0x8D, 0x14, // Charge Pump (enable)
        0x20, 0x00, // Set Memory Addressing Mode to Horizontal
        0xA1, // Set Segment Re-map
        0xC8, // Set COM Output Scan Direction
        0xDA, 0x12, // Set COM Pins Hardware Configuration
        0x81, 0xCF, // Set Contrast Control
        0xD9, 0xF1, // Set Pre-charge Period
        0xDB, 0x40, // Set VCOMH Deselect Level
        0xA4, // Entire Display On (Resume to RAM)
        0xA6, // Set Normal Display
        0xAF  // Turn display on
    };

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Controle de comando
    i2c_master_write(cmd, init_cmds, sizeof(init_cmds), true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
        g_oled_initialized = true;
        oled_clear_framebuffer();
        oled_refresh();
        ESP_LOGI(TAG, "SSD1306 inicializado com sucesso.");
    } else {
        ESP_LOGE(TAG, "Erro ao inicializar SSD1306 via I2C: %s", esp_err_to_name(err));
        g_oled_initialized = false;
    }
}

void displayUpdate(const AppState &state) {
    if (!g_oled_initialized) {
        return;
    }

    oled_clear_framebuffer();

    switch (state.currentScreen) {
        case SCREEN_WATCHFACE: {
            oled_draw_string(0, 4, "=== APPLE WATCH ===");
            
            // Exibicao de hora baseada no time() do sistema (ou ticks)
            time_t now_sec;
            time(&now_sec);
            struct tm timeinfo;
            localtime_r(&now_sec, &timeinfo);
            
            char time_str[32];
            char date_str[32];
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            
            oled_draw_string(2, 40, time_str);
            oled_draw_string(4, 34, date_str);
            
            // Exibicao do status da rede
            char net_str[32];
            snprintf(net_str, sizeof(net_str), "WIFI:%s MQTT:%s", 
                     state.wifiConnected ? "OK" : "--", 
                     state.mqttConnected ? "OK" : "--");
            oled_draw_string(6, 4, net_str);
            break;
        }
        
        case SCREEN_HEALTH: {
            oled_draw_string(0, 4, "=== HEALTH INFOS ===");
            
            // Frequencia Cardiaca
            char hr_str[24];
            if (state.heartRateValid) {
                snprintf(hr_str, sizeof(hr_str), "BPM: %.1f %s", state.heartRateBpm, state.heartRateAlert ? "(!)" : "(OK)");
            } else {
                snprintf(hr_str, sizeof(hr_str), "BPM: Lendo...");
            }
            oled_draw_string(2, 4, hr_str);
            
            // Acelerometro
            char imu_str[24];
            snprintf(imu_str, sizeof(imu_str), "IMU: %.2f G", state.accelMagnitudeG);
            oled_draw_string(4, 4, imu_str);
            
            // Alerta de Queda
            char alert_str[24];
            snprintf(alert_str, sizeof(alert_str), "Queda: %s", state.rapidMotionAlert ? "DETECTADA" : "OK");
            oled_draw_string(6, 4, alert_str);
            break;
        }
        
        case SCREEN_SOS: {
            oled_draw_string(0, 4, "********************");
            oled_draw_string(2, 16, "!!! ALERTA !!!");
            oled_draw_string(3, 22, "MODO DE SOS");
            oled_draw_string(4, 28, "ATIVADO");
            oled_draw_string(6, 4, "********************");
            break;
        }
        
        default:
            oled_draw_string(2, 10, "Tela desconhecida");
            break;
    }

    oled_refresh();
}
