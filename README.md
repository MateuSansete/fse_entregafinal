# Smartwatch IoT — Apple Watch SE 3

**Trabalho Final — Fundamentos de Sistemas Embarcados (2026-1)**

## Participantes do Grupo

| Matrícula | Nome completo |
| :--- | :--- |
| *A complementar* | — |

---

## 1. Descrição do Produto

O produto selecionado para este projeto é o **Apple Watch SE 3 (GPS)**, um smartwatch focado no monitoramento de saúde (sinais vitais, alertas de frequência cardíaca), detecção de incidentes (queda/movimento brusco) e conectividade wireless robusta (Wi-Fi/MQTT).

Nesta versão simplificada com o microcontrolador **ESP32 DevKit**, reproduzimos os recursos essenciais do smartwatch comercial em um protótipo vestível funcional integrado com a plataforma de IoT **ThingsBoard**:
*   **Frequência Cardíaca (BPM):** Medição baseada no sensor analítico PPG (KY-039) acoplado à porta analógica do ESP32 com filtro exponencial digital e alertas no buzzer e na nuvem.
*   **Alerta de Movimento Brusco:** Magnitude de aceleração lida a partir do acelerômetro MPU-6050 e processada pelo firmware para soar o alarme sonoro local caso ultrapasse 2,5 G.
*   **Interface Gráfica Local:** Tela OLED de 0,96" com navegação entre Watchface (relógio/data/status de rede), Health Metrics (dados dos sensores e alertas) e Alerta de SOS.
*   **Navegação Integrada:** Controle das telas através do Rotary Encoder (giro) e botão físico para seleção.
*   **SOS de Emergência:** Acionamento de pânico manual ao pressionar o botão físico por mais de 3 segundos, alterando a tela do relógio e reportando dados imediatos para a nuvem.
*   **Conectividade IoT:** Telemetria estruturada em JSON enviada via Wi-Fi Station e protocolo MQTT ao ThingsBoard, com tratamento de comandos remotos RPC (ligar buzzer remotamente).

---

## 2. Arquitetura da Solução

O smartwatch IoT opera em camadas desde a eletrônica embarcada de leitura de sensores e interface física local até o painel web ThingsBoard de telemetria na nuvem:

```mermaid
graph TD
    subgraph Wearable ESP32 (Hardware & Firmware)
        S1[Sensor KY-039 - ADC] -->|PPG Signal| T_Sensors[sensorTask - Core 1]
        S2[MPU-6050 - I2C] -->|Accel ax,ay,az| T_Sensors
        T_Sensors -->|Mutex Write| State[AppState Shared RAM]
        
        T_UI[uiTask - Core 0] -->|Mutex Read| State
        T_UI -->|Encoder SW & GPIO 4| Inputs[Inputs Debounce]
        T_UI -->|I2C Command| OLED[OLED SSD1306 Display]
        T_UI -->|Bip Sequence| Buzzer[Buzzer Alarme]
        
        T_Network[networkTask - Core 0] -->|Mutex Read/Consume| State
        T_Network -->|Publish WiFi/MQTT| WiFi[Driver STA / esp-mqtt]
    end
    subgraph Cloud IoT
        WiFi -->|JSON Telemetry| TB[ThingsBoard Dashboard]
        TB -->|Remote RPC commands| WiFi
    end
```

### 2.1. Estrutura FreeRTOS

Para coordenar as múltiplas funções críticas do smartwatch de forma assíncrona e não-bloqueante, o firmware utiliza o sistema multitarefas **FreeRTOS** nativo do ESP-IDF:

*   **`sensorTask` (Core 1, Prioridade 2):** Executada a cada 2 ms. Responsável pelo cálculo do PPG cardíaco (amostragem ADC a ~200 Hz) e leitura rápida da aceleração linear (MPU-6050 a ~50 Hz). Rodar no Core 1 garante amostragem sem interrupções por operações de I/O ou display.
*   **`uiTask` (Core 0, Prioridade 1):** Executada a cada 100 ms. Realiza a leitura e debounce dos botões, calcula o delta do encoder rotativo, executa a máquina de estados não-bloqueante do buzzer e renderiza as telas no OLED SSD1306.
*   **`networkTask` (Core 0, Prioridade 1):** Executada a cada 200 ms. Gerencia a conexão Wi-Fi e sincronização MQTT. Publica dados em tempo real a cada 10 segundos ou instantaneamente na ocorrência de quedas/SOS.

#### Mecanismos de Sincronização e Comunicação:
*   **Mutexes (FreeRTOS Mutual Exclusion):** A estrutura de estado centralizada `AppState` é encapsulada em `app_state.cpp` e protegida contra race conditions por um mutex. Todas as leituras e escritas das tarefas acessam o estado através de funções thread-safe (`appStateSnapshot()`, `appStateSetNetwork()`, etc.).
*   **Spinlocks de Hardware (SMP Muxes):** O acumulador do encoder rotativo (`g_encoder_delta`) é alterado no contexto de interrupção (ISR) de descida no GPIO 18 e consumido periodicamente na `uiTask`. O acesso é sincronizado usando `portENTER_CRITICAL(&g_encoder_mux)` para prevenir contenção de barramento entre núcleos.

### 2.2. Sensores e Atuadores (Entradas e Saídas)

| Componente | Tipo (Entrada/Saída) | Pino(s) ESP32 | Função no produto |
| :--- | :--- | :--- | :--- |
| **KY-039 (Batimento)** | Entrada (Analógica) | GPIO 34 (ADC1_CH6) | Leitura do nível de absorção infravermelha para calcular o batimento cardíaco (BPM). |
| **MPU-6050 (Acelerômetro)** | Entrada (I2C) | SDA: **21** / SCL: **22** | Coleta de aceleração linear triaxial para detecção de movimento brusco. |
| **Rotary Encoder (CLK)** | Entrada (Digital + IRQ) | GPIO 18 | Interrupção externa na borda de descida para contagem de pulsos. |
| **Rotary Encoder (DT)** | Entrada (Digital) | GPIO 19 | Leitura de nível lógico para determinar o sentido do giro do encoder. |
| **Encoder Botão (SW)** | Entrada (Digital) | GPIO 23 | Clique do botão integrado no encoder para seleção/confirmação. |
| **Botão SOS / Navegação** | Entrada (Digital) | GPIO 4 | Clique curto avança de tela. Clique longo (>= 3s) dispara o alerta de SOS. |
| **Buzzer Ativo** | Saída (Digital) | GPIO 25 | Alerta acústico intermitente (arritmia, movimento brusco e código Morse de SOS). |
| **OLED 0,96" (SSD1306)** | Saída (I2C) | SDA: **21** / SCL: **22** | Renderização local em tempo real das interfaces do smartwatch. |

---

## 3. Comunicação Wireless

A comunicação do wearable utiliza a pilha TCP/IP do ESP-IDF no modo **Wi-Fi Station (STA)** com reconexão automática e event loop assíncrono.

### 3.1. Integração com Thingsboard / MQTT

*   **Telemetria:** Os dados são publicados periodicamente a cada 10 segundos no tópico `v1/devices/me/telemetry` com o seguinte payload JSON:
    ```json
    {
      "heartRate": 72.5,
      "heartRateAlert": false,
      "accelMagnitude": 1.02,
      "rapidMotionAlert": false,
      "sosTriggered": false
    }
    ```
*   **RPC (comandos remotos):** O smartwatch subscreve-se ao tópico `v1/devices/me/rpc/request/+`. Ao receber o comando `"buzzer"`, ele aciona o bip de teste local por uma duração específica em milissegundos e envia a confirmação de execução para o tópico `v1/devices/me/rpc/response/{request_id}` com o payload `{"success":true}`.

---

## 4. Como Compilar e Executar

### 4.1. Pré-requisitos
*   **ESP-IDF v6.0.1** (ou posterior) devidamente instalado no sistema.
*   Ferramentas do ESP-IDF (como CMake, Ninja, Python 3) ativadas nas variáveis de ambiente.

### 4.2. Configuração
Copie o arquivo de credenciais exemplo para criar seu cabeçalho secreto em `include/secrets.h`:
```bash
cp include/secrets.h.example include/secrets.h
```

Preencha suas informações de Wi-Fi e token do ThingsBoard no arquivo [include/secrets.h](file:///X:/include/secrets.h):
```cpp
#define WIFI_SSID "NOME_DO_WIFI"
#define WIFI_PASSWORD "SENHA_DO_WIFI"
#define TB_ACCESS_TOKEN "SEU_ACCESS_TOKEN"
#define TB_MQTT_HOST "demo.thingsboard.io"
#define TB_MQTT_PORT 1883
```

### 4.3. Compilação e Gravação
Para compilar o projeto de forma nativa e rápida utilizando CMake e Ninja no drive virtual mapeado `X:\`:

```bash
# Configura o projeto para o ESP32
cmake -G Ninja -B build

# Compila o firmware
ninja -C build

# Grava na placa ESP32 (exemplo de porta COM3 no Windows)
esptool.py --chip esp32 -p COM3 write_flash -z 0x10000 build/smartwatch.bin
```

---

## 5. Estrutura do Código

O firmware do smartwatch foi modularizado em subsistemas e integrado com a biblioteca local do cliente MQTT (esp-mqtt) para garantir compilação offline segura:

```text
X:\
├── CMakeLists.txt              # Configurações do projeto e target CMake
├── platformio.ini              # Configuração auxiliar PlatformIO
├── sdkconfig                   # Arquivo de configuração de Kconfig da ESP-IDF
├── include/                    # Diretório de Cabeçalhos
│   ├── app_state.h             # Interface thread-safe do estado do smartwatch
│   ├── buzzer.h                # Definição dos padrões sonoros do buzzer
│   ├── config.h                # Pinos e constantes temporais
│   ├── display_ui.h            # Renderização de menus e fontes SSD1306
│   ├── heart_rate.h            # Configuração ADC e picos cardíacos
│   ├── imu_sensor.h            # Leitura I2C da aceleração
│   ├── inputs.h                # Debouncing de botões e ISR de encoder
│   ├── network.h               # Handlers de Wi-Fi e conexão MQTT
│   ├── secrets.h               # Token ThingsBoard e credenciais Wi-Fi (oculto)
│   └── (headers esp-mqtt...)   # Arquivos de cabeçalho copiados do esp-mqtt
└── src/                        # Código Fonte (.cpp e .c)
    ├── CMakeLists.txt          # Fontes e dependências registradas na ESP-IDF
    ├── main.cpp                # Ponto de entrada (app_main) e tasks FreeRTOS
    ├── app_state.cpp           # Armazenamento e lock de Mutex do estado
    ├── buzzer.cpp              # Sequenciador de passos de alerta sonoro
    ├── display_ui.cpp          # Tabela de fonte 5x7 e desenhos de tela
    ├── heart_rate.cpp          # Filtro exponencial IIR e BPM cardíaco
    ├── imu_sensor.cpp          # Magnitude do acelerômetro e cooldown de queda
    ├── inputs.cpp              # Máquina de cliques de botão e ISR do encoder
    ├── network.cpp             # Máquina de conexão Wi-Fi/MQTT e parsing RPC
    └── (código esp-mqtt...)    # Fontes em C do esp-mqtt compilados localmente
```

---

## 6. Vídeo de Demonstração

O vídeo contendo a demonstração de funcionamento de cada driver, transição de telas através do encoder rotativo, detecção de movimento brusco e recebimento de telemetria no ThingsBoard está disponível no link abaixo:

*   🎥 **Link do vídeo:** [adicionar link do vídeo aqui]

---

## 7. Referências

*   [ESP-IDF Programming Guide — ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
*   [ThingsBoard MQTT API Reference](https://thingsboard.io/docs/reference/mqtt-api/)
*   [SSD1306 Display Controller Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
