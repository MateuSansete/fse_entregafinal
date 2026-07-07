#include "network.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"
#include "secrets.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "NETWORK";

static esp_mqtt_client_handle_t g_mqtt_client = nullptr;
static bool g_wifi_started = false;

// Event handler para o Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Tentando conectar ao Wi-Fi...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Quando Wi-Fi desconecta, redefine ambos para falso
        appStateSetNetwork(false, false);
        ESP_LOGW(TAG, "Wi-Fi desconectado. Tentando reconectar...");
        esp_wifi_connect();
    }
}

// Event handler para o IP
static void ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado! IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        // Wi-Fi conectado, MQTT ainda desconectado
        appStateSetNetwork(true, false);
        
        // Inicia a conexao do cliente MQTT
        if (g_mqtt_client) {
            esp_mqtt_client_start(g_mqtt_client);
        }
    }
}

// Event handler para o MQTT
static void mqtt_event_handler(void* handler_args, esp_event_base_t base, 
                               int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao broker MQTT ThingsBoard!");
            // Wi-Fi e MQTT conectados
            appStateSetNetwork(true, true);
            
            // Subscreve ao topico de comandos RPC do ThingsBoard
            esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 1);
            ESP_LOGI(TAG, "Subscrito no topico RPC.");
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Broker MQTT desconectado.");
            // Wi-Fi ainda conectado, mas MQTT desconectado
            appStateSetNetwork(true, false);
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "Mensagem MQTT recebida no topico: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Dados: %.*s", event->data_len, event->data);
            
            // Processamento de comandos RPC (Buzzer remoto)
            const char *rpc_prefix = "v1/devices/me/rpc/request/";
            int prefix_len = strlen(rpc_prefix);
            if (event->topic_len > prefix_len && strncmp(event->topic, rpc_prefix, prefix_len) == 0) {
                char *data_buf = (char*)malloc(event->data_len + 1);
                if (data_buf) {
                    memcpy(data_buf, event->data, event->data_len);
                    data_buf[event->data_len] = '\0';
                    
                    // Verifica se o metodo solicitado e o acionamento do buzzer
                    if (strstr(data_buf, "\"method\":\"buzzer\"") != nullptr || 
                        strstr(data_buf, "\"method\": \"buzzer\"") != nullptr) {
                        
                        // Extrai a duracao dos parametros
                        int duration = 1000;
                        char *params_ptr = strstr(data_buf, "\"params\"");
                        if (params_ptr) {
                            char *colon_ptr = strchr(params_ptr, ':');
                            if (colon_ptr) {
                                int val = atoi(colon_ptr + 1);
                                if (val > 0) {
                                    duration = val;
                                }
                            }
                        }
                        
                        // Dispara o bip no buzzer local
                        buzzerBeep(duration);
                        ESP_LOGI(TAG, "RPC Comando: Buzzer acionado remotamente por %d ms.", duration);
                        
                        // Envia resposta de sucesso ao ThingsBoard
                        char response_topic[64];
                        snprintf(response_topic, sizeof(response_topic), "v1/devices/me/rpc/response/%.*s", 
                                 (int)(event->topic_len - prefix_len), event->topic + prefix_len);
                        esp_mqtt_client_publish(client, response_topic, "{\"success\":true}", 0, 1, 0);
                    }
                    free(data_buf);
                }
            }
            break;
        }
            
        default:
            break;
    }
}

void networkInit() {
    // 1. Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar o NVS: %s", esp_err_to_name(ret));
        return;
    }

    // 2. Inicializa a pilha de rede TCP/IP e loop de eventos
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro esp_netif_init: %s", esp_err_to_name(ret));
        return;
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Erro esp_event_loop_create_default: %s", esp_err_to_name(ret));
        return;
    }

    // 3. Cria a interface Wi-Fi Station padrao
    esp_netif_create_default_wifi_sta();

    // 4. Inicializa o Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro esp_wifi_init: %s", esp_err_to_name(ret));
        return;
    }

    // 5. Registra os handlers de eventos do Wi-Fi e IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, nullptr, nullptr));

    // 6. Configura o SSID e Senha do Wi-Fi Station
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // 7. Inicia o Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());
    g_wifi_started = true;
    ESP_LOGI(TAG, "Wi-Fi iniciado no modo Station (SSID: %s).", WIFI_SSID);

    // 8. Inicializa e configura o cliente MQTT do ThingsBoard
    esp_mqtt_client_config_t mqtt_cfg = {};
    static char uri_buf[128];
    snprintf(uri_buf, sizeof(uri_buf), "mqtt://%s:%d", TB_MQTT_HOST, TB_MQTT_PORT);
    mqtt_cfg.broker.address.uri = uri_buf;
    mqtt_cfg.credentials.username = TB_ACCESS_TOKEN;

    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (g_mqtt_client) {
        esp_mqtt_client_register_event(g_mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
        ESP_LOGI(TAG, "Cliente MQTT do ThingsBoard instanciado (%s). Aguardando Wi-Fi...", uri_buf);
    } else {
        ESP_LOGE(TAG, "Erro ao instanciar o cliente MQTT.");
    }
}

void networkUpdate() {
    static uint32_t last_publish_time = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    AppState state = appStateSnapshot();
    
    // Consome requisicao de envio imediato (ex: queda ou SOS) ou verifica o timeout periodico
    bool publish_needed = appStateConsumePublishRequest() || (now - last_publish_time >= MQTT_TELEMETRY_INTERVAL_MS);
    
    if (publish_needed && state.mqttConnected && g_mqtt_client) {
        char payload[384];
        uint32_t uptime_s = now / 1000;
        snprintf(payload, sizeof(payload),
                 "{\"heart_rate_bpm\":%.1f,\"heart_rate_alert\":%s,\"accel_magnitude_g\":%.2f,\"rapid_motion_alert\":%s,\"sos_triggered\":%s,\"sos_manual\":%s,\"screen\":\"%s\",\"uptime_s\":%lu}",
                 state.heartRateValid ? state.heartRateBpm : 0.0f,
                 state.heartRateAlert ? "true" : "false",
                 state.accelMagnitudeG,
                 state.rapidMotionAlert ? "true" : "false",
                 state.sosTriggered ? "true" : "false",
                 state.sosManual ? "true" : "false",
                 screenName(state.currentScreen),
                 (unsigned long)uptime_s);
                 
        int msg_id = esp_mqtt_client_publish(g_mqtt_client, "v1/devices/me/telemetry", payload, 0, 1, 0);
        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Telemetria publicada com sucesso! ID: %d, Payload: %s", msg_id, payload);
            last_publish_time = now;
        } else {
            ESP_LOGE(TAG, "Falha ao publicar telemetria no ThingsBoard.");
        }
    }
}

