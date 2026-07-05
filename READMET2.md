# Smartwatch IoT

**Trabalho 2 - 2026-1**

## Integrantes do Grupo

| Nome Completo | Matrícula |
| :--- | :--- |
| Mateus Bastos dos Santos | 211062240 |
| Arthur Heleno do Couto da Silva | 180116746 |
| Luis Henrique Luz Costa | 180066161 |
| Daniel Coimbra dos Santos | 180113097 |

---

## 1. Descrição do Produto Selecionado

<p align="center">
  <img src="appleWatch.png" alt="Apple Watch SE 3" width="400"/>
</p>
<p align="center"><em>Figura 1: Apple Watch SE 3 comercial, smartwatch de referência para saúde, atividade física e conectividade.</em></p>

O produto selecionado para estudo é o **Apple Watch SE (3ª geração)**, um smartwatch da Apple lançado em setembro de 2024. Trata-se da versão de entrada da linha Apple Watch, mas compartilhando o mesmo processador S9 SiP dual-core de 64 bits presente na Series 9. 

O dispositivo conta com tela Retina LTPO OLED de 40 ou 44 mm com brilho de até 1.000 nits, vidro Ion-X resistente a arranhões, caixa em alumínio reciclado, sensor óptico de frequência cardíaca (PPG de dupla frequência, verde e infravermelha), acelerômetro, giroscópio, altímetro barométrico sempre ativo, conectividade Wi-Fi 802.11b/g/n/ax (2,4 GHz e 5 GHz), Bluetooth 5.3 e resistência à água de 50 metros (WR50). Funciona em conjunto com o iPhone (iOS 17 ou superior) e executa o watchOS 11.

### O que o Apple Watch SE 3 faz, Funções Principais, Público-Alvo e Contexto de Uso

O Apple Watch SE 3 é voltado para usuários comuns e praticantes de atividades físicas que buscam monitorar a saúde no dia a dia, receber notificações no pulso e ter recursos de segurança pessoal, sem precisar pagar pelos sensores de nicho dos modelos premium (como ECG ou medição de temperatura basal). O contexto de uso é estritamente vestível (*wearable*).

**Monitoramento de saúde contínuo:** O relógio monitora a frequência cardíaca de forma passiva e contínua ao longo do dia, alertando o usuário quando a FC sobe acima de 120 bpm ou cai abaixo de 40 bpm em repouso. O sensor óptico também detecta ritmos cardíacos irregulares sugestivos de fibrilação atrial e notifica o usuário para procurar avaliação médica. Adicionalmente, o dispositivo mede a saturação de oxigênio no sangue (SpO₂), monitora os ciclos de sono identificando fases REM, núcleo e sono profundo, e rastreia sinais de apneia do sono por meio de análise de acelerometria noturna. O altímetro barométrico registra lances de escada subidos ao longo do dia.

**Atividade física e esportes:** Durante exercícios ao ar livre, o Apple Watch SE 3 utiliza o GPS do iPhone pareado via Bluetooth para traçar rotas e calcular distância.

**Segurança pessoal e emergências:** Um dos recursos relevantes do Apple Watch SE 3 é a **detecção de queda**: o acelerômetro e o giroscópio analisam continuamente padrões de movimento; quando detectam um impacto, o relógio toca um alarme e oferece ao usuário a opção de ligar para o serviço de emergência local.  **SOS de Emergência** permite pressionar e segurar o botão lateral para acionar socorro imediatamente. O **Compartilhamento de Localização com a Família** permite que pais acompanhem filhos ou cuidadores monitorem idosos em tempo real.

**Conectividade e conveniência:** Exibe notificações de mensagens, ligações, e-mails e aplicativos diretamente no pulso, sem tirar o iPhone do bolso. O Apple Pay via NFC permite pagamentos por aproximação sem o iPhone. O microfone integrado possibilita atender ligações diretamente no relógio quando pareado com AirPods ou EarPods.

**Ecossistema e integração:** O watchOS 11 introduziu o app Vitalidade, que combina dados de sono, atividade, frequência cardíaca e histórico de saúde para gerar uma pontuação diária de prontidão física. O relógio também serve como controle remoto para câmera, Apple TV e reprodução de música.

### Motivo da Escolha do Produto

O Apple Watch SE 3 foi escolhido como produto de referência por reunir, em um único dispositivo, exatamente as três camadas tecnológicas que este trabalho propõe estudar: **aquisição de sinais biométricos** (sensor PPG para frequência cardíaca), **processamento de dados inerciais** (acelerômetro para detecção de queda e contagem de passos) e **conectividade sem fio com envio de dados para a nuvem** (Wi-Fi + Bluetooth + integração com app e serviços remotos). Essas três camadas têm correspondência direta com os componentes do nosso protótipo: sensor KY-039 (PPG), MPU-6050 (acelerômetro) e ESP32 com Wi-Fi/MQTT.

Por fim, o Apple Watch SE 3 possui **ampla documentação pública** — especificações técnicas no site da Apple, artigos científicos sobre seus sensores e código-fonte de apps disponível para estudo. O que facilitou o levantamento bibliográfico e a definição do escopo de implementação.

Neste projeto, o objetivo é replicar de forma simplificada as funções essenciais desse produto como veremos a seguir.

### Componentes e Sensores Utilizados (Produto Original)

Com base nas especificações técnicas do produto comercial, o Apple Watch SE 3 utiliza:

* **Sensor óptico de frequência cardíaca:** Utiliza tecnologia PPG (fotopletismografia) com LEDs verdes e infravermelhos para medir o fluxo sanguíneo no pulso.
* **Acelerômetro de alta dinâmica e Giroscópio:** Essenciais para a contagem de passos, detecção de movimento do pulso para acender a tela e para o algoritmo de detecção de queda.
* **Altímetro barométrico:** Usado para medir ganho de elevação (andares subidos).
* **Motor háptico (Taptic Engine):** Atuador que gera vibrações precisas para notificações físicas no pulso.
* **Microfone e Alto-falante:** Para chamadas, interação com a Siri e alarmes de emergência.
* **Tecnologias de Comunicação:** Wi-Fi 802.11b/g/n (2,4 GHz e 5 GHz), Bluetooth 5.3 e chip NFC.

---

## 2. Análise Técnica do Funcionamento

### Principais Módulos do Sistema Comercial

O funcionamento do Apple Watch SE 3 pode ser dividido em grandes blocos operacionais gerenciados pelo chip principal (S9 SiP):

* **Módulo de Aquisição Biométrica:** O sensor PPG faz amostragens passivas e contínuas. Quando o usuário está em repouso, o sistema usa luz infravermelha para economizar bateria. Durante exercícios, aciona os LEDs verdes, que têm maior precisão para medir frequências cardíacas elevadas.
* **Módulo de Análise Inercial:** O acelerômetro e o giroscópio trabalham em conjunto o tempo todo. Para a função de detecção de queda, eles analisam a assinatura do impacto (magnitude da aceleração) e a orientação espacial. Se detectarem um impacto forte seguido de imobilidade, acionam o módulo de emergência.
* **Módulo de Comunicação com o Ecossistema:** O relógio atua como um nó que coleta os dados (Edge) e os repassa via Bluetooth Low Energy para o iPhone pareado, que faz o processamento mais pesado (como traçar rotas de GPS) e joga as informações consolidadas no iCloud (app Saúde).

### Identificação de Tecnologias Críticas

* **Integração SiP (System in Package):** Colocar processador, memória, controladores wireless e de energia em um único encapsulamento selado para economizar espaço físico.
* **Fotopletismografia (PPG) com DSP:** A filtragem digital de sinais é vital para limpar o ruído gerado pelo movimento do braço e conseguir ler os batimentos apenas pelas variações de luz.
* **Algoritmos de Machine Learning no Edge:** Usados para a detecção automática de exercícios e diferenciação de movimentos reais de quedas falsas (para não ligar para a emergência se o usuário apenas bater na mesa).

---

## 3. Proposta de Reprodução com ESP32

Neste projeto, o objetivo é recriar o conceito central do smartwatch analisado: **monitorar batimentos cardíacos, detectar movimentos bruscos (simulando a ideia de detecção de queda) e expor esses dados remotamente.** Para isso, substituímos o ecossistema fechado da Apple por componentes acessíveis centrados no microcontrolador ESP32 e na plataforma IoT ThingsBoard. O dashboard web funcionará como o aplicativo "Saúde" do celular.

### 3.1. Fluxo de Funcionamento do Protótipo

Nosso firmware irá rodar três lógicas principais em paralelo (via tasks do FreeRTOS ou timers):

1. **Monitoramento Cardíaco e Alertas:** O sensor de pulso faz a leitura via porta analógica. A ESP32 calcula os BPMs. Se o valor ficar abaixo de 50 ou acima de 120 por 5 segundos seguidos, o relógio emite um alerta sonoro no buzzer local e envia um aviso para o ThingsBoard.
2. **Alerta de Movimento Brusco:** A ESP32 lê continuamente os dados do acelerômetro do módulo IMU. Se a magnitude da aceleração passar de um limite configurado (ex: 2,5g), o buzzer apita e o sistema manda uma flag de emergência (`rapid_motion_alert: true`) via MQTT. 
3. **Interface e SOS Manual:** O usuário pode navegar nas telas do display OLED usando o encoder rotativo. Caso pressione e segure o botão por mais de 3 segundos, o sistema ignora os sensores e dispara um alerta de "SOS Manual" direto para o painel web.

*(Nota: O projeto não tenta emular os algoritmos complexos de impacto do relógio original, atuando apenas em picos de aceleração simples).*

### 3.2. Componentes e Sensores Utilizados (Hardware do Projeto)

A montagem foi dimensionada para um único ESP32 DevKit, utilizando barramento I2C compartilhado para otimizar os pinos.

| Componente | Interface | GPIO na ESP32 | Função no Protótipo |
| :--- | :--- | :--- | :--- |
| **Tela OLED 0,98"** | I2C | SDA **21**, SCL **22** | Exibir a interface local (hora, BPM e menus). |
| **IMU (MPU-6050)** | I2C | SDA **21**, SCL **22** | Acelerômetro para medir movimentação brusca. |
| **Sensor de Pulso (KY-039)**| ADC | **34** | Ler o fluxo sanguíneo para cálculo de BPM. |
| **Encoder Rotativo** | Digital + IRQ | **18** (CLK), **19** (DT) | Navegação (scroll) entre as telas. |
| **Botões** | Digital | **23** (SW), **4** (Botão Extra)| Seleção de itens e acionamento do modo SOS. |
| **Buzzer Ativo** | Digital | **25** | Feedback sonoro local de alertas e alarmes. |
| **Bateria 9V + Step-down** | Alimentação| GND, 3.3V | Converter energia para a tensão lógica do circuito. |

### 3.3. O que a Eletrônica Permite Medir no ThingsBoard

| Grandeza Física | Campo MQTT | Unidade | Dashboard (ThingsBoard) |
| :--- | :--- | :--- | :--- |
| **Frequência cardíaca** | `heart_rate_bpm` | bpm | Gauge + gráfico de linha |
| **Alerta FC** | `heart_rate_alert` | bool | Alarme / LED virtual |
| **Magnitude de aceleração** | `accel_magnitude_g` | g | Gráfico de linha |
| **Movimento brusco** | `rapid_motion_alert` | bool | Alarme de evento |
| **SOS manual** | `sos_triggered` | bool | Alarme crítico |

### 3.4. Diagrama Conceitual do Sistema

O diagrama abaixo ilustra a arquitetura em camadas do nosso protótipo, desde a aquisição dos sinais físicos no hardware (Edge), passando pelo processamento no firmware (ESP32), até a transmissão via rede (MQTT) para a camada de aplicação no ThingsBoard.

<p align="center">
  <img src="appleDiagram.png" alt="Diagrama conceitual do smartwatch IoT com ESP32 e ThingsBoard" width="800"/>
</p>
<p align="center"><em>Figura 2: Diagrama conceitual de arquitetura em camadas — hardware, firmware, rede e ThingsBoard.</em></p>

### 3.5. Limitações e Desafios Esperados

A reprodução com componentes de laboratório traz desafios óbvios se comparados com a engenharia de precisão da Apple:

* **Precisão Biométrica:** O sensor óptico KY-039 é estritamente educacional. Movimentos leves no dedo ou luz ambiente alteram muito a leitura, tornando o cálculo de BPM ruidoso. Exigirá calibração e filtro de média móvel por software.
* **Sensibilidade de Movimento:** Movimentos normais do braço podem ultrapassar o limiar de 2,5g. Será um desafio calibrar esse limite (`ACCEL_LIMIT`) na prática para não gerar "alarmes falsos" de queda.
* **Autonomia de Energia:** Uma bateria padrão de 9V durará apenas algumas horas por conta do pico de consumo contínuo da comunicação Wi-Fi do módulo ESP32, servindo apenas como Prova de Conceito (PoC).
* **Fator de Forma:** O uso de módulos soltos em protoboard impede o uso real do dispositivo de forma presa ao pulso para esportes.

---



## 4. Pesquisa Bibliográfica e Tecnológica

## 4.1 Artigos científicos sobre tecnologias que viabilizam a existência do Apple Watch SE 3

### 4.1.1 Artigos sobre Wi-Fi / IEEE 802.11 / modo STA

**1.** **IEEE 802.11 Wireless Local Area Networks**

**Link:** https://doi.org/10.1109/35.620533

**Referência Bibliográfica:**

CROW, B. P.; WIDJAJA, I.; KIM, J. G.; SAKAI, P. T. IEEE 802.11 wireless local area networks. IEEE Communications Magazine, v. 35, n. 9, p. 116–126, 1997. DOI: 10.1109/35.620533. (Kyung Hee University)

**Breve Resumo**

É apresentada no artigo, datado dos primórdios da Internet (pois foi publicado em 1997), uma explicação inicial do padrão IEEE 802.11, que serviu como base para as redes Wi-Fi atuais. O artigo aborda a subcamada MAC, responsável por controlar como os dispositivos acessam o meio sem fio para transmitir dados sem causar colisões excessivas. É analisado o desempenho da rede para dois tipos de tráfego: dados comuns em pacotes e voz transmitida sobre a WLAN. Os autores investigam se o padrão seria capaz de atender aplicações como comunicação por voz com restrições de tempo. A função de coordenação pontual (Point Coordination Function) é apresentada como um mecanismo capaz de ajudar no transporte de tráfego sensível ao tempo. Essa função organiza o acesso dos dispositivos ao meio de forma mais controlada, o que pode reduzir atrasos em aplicações que exigem maior previsibilidade. O artigo conclui que, embora a rede Wi-Fi possa transportar tráfego de voz, esse tipo de aplicação exige cuidados adicionais e, em especial, os autores indicam a necessidade de uso de cancelador de eco para melhorar a qualidade dessa comunicação.

**Aplicação relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso do IEEE 802.11/Wi-Fi como tecnologia de comunicação sem fio para permitir que dispositivos embarcados troquem dados em rede. No contexto do projeto do smartwatch com ESP32, esse artigo ajuda a justificar o uso do Wi-Fi em modo STA, no qual o dispositivo embarcado se conecta a um roteador/ponto de acesso para enviar telemetria, como BPM e alertas, para uma plataforma remota como o ThingsBoard. Em resumo: o artigo fundamenta a camada de comunicação sem fio que permite ao sistema embarcado sair do funcionamento isolado e se integrar a uma rede.

**2.** **IEEE 802.11ah: A Technology to Face the IoT Challenge**

**Link:** https://doi.org/10.3390/s16111960

**Referência Bibliográfica:**

BAÑOS-GONZALEZ, V.; AFAQUI, M. S.; LOPEZ-AGUILERA, E.; GARCIA-VILLEGAS, E. IEEE 802.11ah: a technology to face the IoT challenge. Sensors, v. 16, n. 11, artigo 1960, 2016. DOI: 10.3390/s16111960. (MDPI)

**Breve Resumo:**

É analisado no artigo o padrão IEEE 802.11ah, uma evolução da família de padrões Wi-Fi pensada para atender às necessidades da Internet das Coisas (IoT). Diferentemente das redes Wi-Fi tradicionais, voltadas principalmente para computadores, smartphones e aplicações de alta taxa de dados, o IEEE 802.11ah busca atender dispositivos IoT, que geralmente exigem baixo consumo de energia, maior alcance e suporte a muitos dispositivos conectados. Um ponto central é a comparação do padrão com outros padrões IEEE 802.11. Os autores mostram que esse padrão foi pensado para superar limitações das versões tradicionais do Wi-Fi quando aplicado a cenários de IoT, como sensores, dispositivos embarcados, automação residencial, monitoramento remoto e aplicações industriais. É também destacado que o padrão compete com outras tecnologias de comunicação para IoT, mesmo assim, os resultados apresentados indicam que ele possui características adequadas para esse tipo de aplicação, especialmente por combinar conectividade sem fio, suporte a muitos dispositivos e recursos voltados à eficiência energética. A importância do trabalho está em mostrar que o Wi-Fi, por meio do padrão IEEE 802.11ah, pode ser adaptado para aplicações de IoT e sistemas embarcados.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso do IEEE 802.11ah como tecnologia Wi-Fi voltada para IoT, permitindo comunicação sem fio com baixo consumo de energia, maior alcance e suporte a muitos dispositivos conectados. No contexto do smartwatch com ESP32, o artigo ajuda a justificar a importância de padrões Wi-Fi adaptados a dispositivos embarcados e vestíveis, que precisam enviar dados de sensores para a rede sem gastar muita energia. Em resumo, o artigo fundamenta o uso de Wi-Fi em sistemas embarcados IoT, especialmente quando há sensores, bateria limitada e envio de telemetria para uma plataforma remota.

---

### 4.1.2 Artigos sobre MQTT

**1.** **MQTT Protocol: Fundamentals, Tools and Future Directions**

**Link:** https://doi.org/10.1109/TLA.2019.8931137

**Referência Bibliográfica:**

QUINCOZES, Silvio E.; EMILIO, Tubino; KAZIENKO, Juliano F. MQTT Protocol: Fundamentals, Tools and Future Directions. IEEE Latin America Transactions, v. 17, n. 9, p. 1439–1448, 2019. DOI: 10.1109/TLA.2019.8931137. (IEEE Latin America Transactions)

**Breve Resumo:**

É apresentado o MQTT como um protocolo de comunicação relevante para aplicações da Internet das Coisas, principalmente por permitir troca eficiente de mensagens entre dispositivos com recursos limitados. Ele é investigado como uma solução de camada de aplicação para sistemas IoT que precisam transmitir dados de forma leve, confiável e adequada a ambientes com restrições de energia e processamento. Um ponto importante é a comparação entre o MQTT e outros protocolos de IoT, especialmente o CoAP. Essa comparação permite entender vantagens e limitações de cada abordagem em termos de desempenho, atraso de comunicação, interoperabilidade e adequação a diferentes cenários de aplicação. É também abordado o MQTT-SN, uma variação do MQTT voltada para redes de sensores. Essa versão é importante porque adapta o protocolo para dispositivos ainda mais restritos, comuns em sistemas embarcados e redes de sensores sem fio. Outro aspecto relevante é a preocupação com desafios como segurança, qualidade de serviço, eficiência energética e interoperabilidade. Por fim, o trabalho contribui ao reunir fundamentos, ferramentas, experimentos e desafios futuros relacionados ao MQTT, servindo como uma base teórica para justificar o uso desse protocolo em sistemas embarcados conectados à nuvem.

**Aplicação relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso do MQTT como protocolo leve de comunicação IoT, permitindo que dispositivos com recursos limitados enviem dados para uma plataforma remota. No contexto do smartwatch com ESP32, o MQTT permite publicar telemetria como BPM, aceleração e alertas cardíacos, movimento brusco e SOS para o ThingsBoard. Em resumo, o artigo justifica o MQTT como tecnologia adequada para comunicação entre sensores embarcados e sistemas em nuvem, por ser leve, eficiente e próprio para IoT.

**2.** **MQTT Implementations, Open Issues, and Challenges: A Detailed Comparison and Survey**

**Link:** https://doi.org/10.2174/2210327913666221216152446

**Referência Bibliográfica:**

AKSHATHA, P. S.; DILIP KUMAR, S. M.; VENUGOPAL, K. R. MQTT Implementations, Open Issues, and Challenges: A Detailed Comparison and Survey. International Journal of Sensors, Wireless Communications and Control, v. 12, n. 8, p. 553–576, 2022. DOI: 10.2174/2210327913666221216152446. (Bentham Direct)

**Breve Resumo**

É apresentado o MQTT como um protocolo de comunicação leve, aberto e adequado para aplicações da Internet das Coisas. Sua arquitetura baseada em publicação/assinatura facilita a troca de mensagens entre sensores, atuadores, aplicações e brokers, o que o torna especialmente útil em sistemas com baixo poder de processamento, pouca memória ou restrições de largura de banda. Uma parte importante é a comparação do MQTT com outros protocolos da camada de aplicação. O artigo discute vantagens e limitações do MQTT, além de apresentar sua relação com o MQTT-SN. São também abordados aspectos práticos, como ferramentas disponíveis, brokers MQTT e aplicações reais do protocolo. Por fim, o trabalho contribui ao organizar o conhecimento sobre MQTT por meio de tabelas comparativas, estatísticas, avaliação de desempenho e uma taxonomia de protocolos da camada de aplicação.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso do MQTT em dispositivos IoT com recursos limitados, como sensores, atuadores, redes de sensores sem fio e comunicação M2M. No contexto do smartwatch com ESP32, o artigo ajuda a justificar a escolha do MQTT para enviar dados de BPM, aceleração e alertas ao ThingsBoard, usando uma arquitetura leve baseada em broker e no modelo publish/subscribe. Em resumo, o artigo mostra que o MQTT é adequado para sistemas embarcados porque consome pouca largura de banda, é simples de implementar e funciona bem em dispositivos conectados com limitações de processamento, memória e energia.

---

### 4.1.3 Artigo sobre Fotopletismografia (PPG) — Tecnologia do Sensor de Frequência Cardíaca

**3.** **Photoplethysmography and Its Application in Clinical Physiological Measurement**

**Link:** https://doi.org/10.1088/0967-3334/28/3/R01

**Referência Bibliográfica:**

ALLEN, John. Photoplethysmography and its application in clinical physiological measurement. Physiological Measurement, v. 28, n. 3, p. R1–R39, 2007. DOI: 10.1088/0967-3334/28/3/R01. (IOP Publishing)

**Breve Resumo**

O artigo apresenta uma revisão abrangente da fotopletismografia (PPG), técnica óptica não invasiva utilizada para medir variações de volume sanguíneo nos tecidos periféricos. O princípio de funcionamento baseia-se na emissão de luz (geralmente infravermelha ou verde) sobre a pele e na detecção da luz refletida ou transmitida: quando o coração bombeia sangue, o volume de sangue nos capilares varia ciclicamente, alterando a quantidade de luz absorvida e, assim, produzindo um sinal elétrico periódico que representa os batimentos cardíacos. O trabalho descreve os fundamentos físicos do sinal PPG, os tipos de sensores, os modos de operação (transmissão e reflexão), as fontes de ruído (como artefatos de movimento e variações de iluminação ambiente) e os métodos de processamento do sinal. São também discutidas as aplicações clínicas da PPG, incluindo oximetria de pulso, avaliação da função cardiovascular, monitoramento da frequência cardíaca e análise da variabilidade da frequência cardíaca (HRV). O artigo ressalta que, apesar de sua simplicidade construtiva, o sinal PPG carrega informações fisiológicas ricas e é sensível a interferências mecânicas, o que exige filtragem e algoritmos de detecção de picos robustos para uso confiável.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está na implementação de sensores ópticos de frequência cardíaca (tipo PPG) em dispositivos vestíveis com microcontroladores de baixo custo. No contexto do smartwatch com ESP32, o sensor de pulso KY-039 aplica exatamente o princípio PPG por reflexão: a saída analógica (AO) fornece o sinal de variação de volume sanguíneo ao ADC do ESP32 (GPIO 34), que aplica filtro passa-banda e detecção de picos para calcular o BPM. O artigo justifica tanto a escolha do sensor como as etapas de filtragem e debounce previstas no firmware, e alerta para a sensibilidade do sinal PPG a artefatos de movimento — desafio identificado na Seção 3.3 deste trabalho.

---

## 4.2 Artigos científicos sobre uso e aplicação do Apple Watch

**1.** **Large-Scale Assessment of a Smartwatch to Identify Atrial Fibrillation**

**Link:** https://doi.org/10.1056/NEJMoa1901183

**Referência Bibliográfica:**

PEREZ, Marco V. et al. Large-scale assessment of a smartwatch to identify atrial fibrillation. The New England Journal of Medicine, v. 381, n. 20, p. 1909–1917, 2019. DOI: 10.1056/NEJMoa1901183. (PubMed)

**Breve Resumo**

O artigo apresenta um estudo que avalia, em larga escala, se um smartwatch com sensores ópticos, especificamente o Apple Watch, poderia identificar sinais sugestivos de fibrilação atrial durante o uso cotidiano. O estudo envolveu mais de 419 mil participantes, o que o torna uma das maiores avaliações sobre o uso de smartwatch para monitoramento cardíaco. O funcionamento estudado baseia-se na detecção passiva de irregularidades no pulso por meio de sensores ópticos. Quando o algoritmo identificava um possível pulso irregular, o participante recebia uma notificação, passava por uma etapa de telemedicina e recebia um adesivo de ECG para confirmação. Um dos resultados mais importantes foi que apenas 0,52% dos participantes receberam notificação de pulso irregular, indicando que o sistema não gerou notificações em grande escala de forma indiscriminada. Entre os participantes que devolveram ECGs analisáveis, 34% apresentaram fibrilação atrial nas leituras posteriores. Outro ponto relevante é que, quando uma nova notificação de pulso irregular ocorria simultaneamente ao ECG, a concordância com fibrilação atrial foi alta, com valor preditivo positivo de 84%. Isso sugere que o algoritmo tinha boa capacidade de indicar episódios compatíveis com fibrilação atrial no momento da notificação. O estudo também mostra a importância dos dispositivos vestíveis conectados à Internet para pesquisas e monitoramento remoto. O modelo utilizado dispensou visitas presenciais, combinando smartwatch, aplicativo, telemedicina e envio remoto de dados, o que demonstra o potencial de sistemas embarcados vestíveis em aplicações de saúde digital.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso de sensores ópticos em dispositivos vestíveis para monitorar sinais fisiológicos, como pulso e frequência cardíaca, de forma contínua ou periódica. No contexto do smartwatch com ESP32, o artigo justifica a implementação de um sistema embarcado que lê um sensor de batimentos cardíacos, processa o sinal, identifica possíveis irregularidades e envia alertas para uma plataforma remota. Em resumo, o artigo mostra como um smartwatch pode combinar sensor biométrico, algoritmo embarcado, conectividade e alerta remoto para monitoramento de saúde.

**2.** **Accuracy of Apple Watch Measurements for Heart Rate and Energy Expenditure in Patients With Cardiovascular Disease: Cross-Sectional Study**

**Link:** https://doi.org/10.2196/11889

**Referência Bibliográfica:**

FALTER, Maarten et al. Accuracy of Apple Watch measurements for heart rate and energy expenditure in patients with cardiovascular disease: cross-sectional study. JMIR mHealth and uHealth, v. 7, n. 3, e11889, 2019. DOI: 10.2196/11889. (PubMed)

**Breve Resumo**

O artigo avalia a precisão do Apple Watch em pacientes com doença cardiovascular, comparando suas medições com métodos clínicos de referência. A frequência cardíaca medida pelo Apple Watch foi comparada com um ECG de 12 derivações, enquanto o gasto energético foi comparado com calorimetria indireta. O resultado mais importante é que o Apple Watch apresentou precisão clinicamente aceitável para medir frequência cardíaca durante o exercício. Isso sugere que dispositivos vestíveis podem ser úteis em programas de acompanhamento e reabilitação cardíaca, especialmente quando o objetivo é monitorar a intensidade do exercício por meio da frequência cardíaca. Por outro lado, o artigo mostra uma limitação relevante: o Apple Watch superestimou sistematicamente o gasto energético. Assim, embora o dispositivo tenha apresentado bom desempenho para frequência cardíaca, seus dados de gasto calórico devem ser interpretados com cautela, principalmente em pacientes com doenças cardiovasculares.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso de sensores vestíveis para medir frequência cardíaca durante atividades físicas, com processamento local dos dados e comparação com limites ou padrões esperados. No contexto do smartwatch com ESP32, o artigo justifica a implementação de um sistema que mede BPM em tempo real, exibe o valor no OLED e envia os dados para o ThingsBoard. Em resumo, o artigo mostra que a medição de frequência cardíaca em smartwatches é uma aplicação prática de sistemas embarcados em saúde, mas também reforça que essas medições precisam ser tratadas com cautela, pois nem sempre têm precisão clínica.

**3.** **Validity and Reliability of the Apple Watch for Measuring Heart Rate During Exercise**

**Link:** https://doi.org/10.1055/s-0043-120195

**Referência Bibliográfica:**

KHUSHHAL, Alaa et al. Validity and reliability of the Apple Watch for measuring heart rate during exercise. Sports Medicine International Open, v. 1, n. 6, p. E206–E211, 2017. DOI: 10.1055/s-0043-120195. (Coventry University)

**Breve Resumo**

O artigo avalia se o sensor de frequência cardíaca do Apple Watch mede a frequência cardíaca de forma válida e confiável durante diferentes intensidades de exercício. Para isso, os autores compararam as medições do Apple Watch com um monitor Polar S810i, usado como referência. O estudo mostrou que o Apple Watch apresentou desempenho muito bom durante caminhada, com correlação muito alta em relação ao equipamento de referência. No entanto, conforme a intensidade do exercício aumentou, passando para trote e corrida, a validade das medições diminuiu. Um ponto importante é que, em exercícios mais intensos, houve redução na quantidade de leituras de frequência cardíaca registradas pelo Apple Watch. Isso indica que movimentos mais intensos podem dificultar a leitura do sensor óptico no pulso, provavelmente por causa de deslocamento do relógio, ruído no sinal ou menor estabilidade do contato com a pele. O artigo também destaca que a confiabilidade entre dois Apple Watches usados simultaneamente foi boa, o que sugere consistência entre dispositivos. Ainda assim, a precisão não foi igual em todas as condições.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso de sensores ópticos de frequência cardíaca em dispositivos vestíveis, com processamento local do sinal durante atividades físicas. No contexto do smartwatch com ESP32, o artigo justifica a necessidade de medir BPM, tratar ruídos causados por movimento e exibir/enviar os dados em tempo real. Em resumo, o artigo mostra que a medição de frequência cardíaca em smartwatches é viável, mas sua precisão pode diminuir com movimentos mais intensos, reforçando a importância de calibração, filtragem e validação do sensor no protótipo.

**4.** **Sensitivity of Apple Watch Fall Detection Feature Among Wheelchair Users**

**Link:** https://doi.org/10.1080/10400435.2021.1923087

**Referência Bibliográfica:**

ABOU, Libak et al. Sensitivity of Apple Watch fall detection feature among wheelchair users. Assistive Technology, v. 34, n. 5, p. 619–625, 2022. DOI: 10.1080/10400435.2021.1923087. (PubMed)

**Breve Resumo**

O artigo avalia a capacidade do Apple Watch de detectar quedas em um contexto específico: quedas a partir de uma cadeira de rodas. O estudo foi realizado em laboratório com 25 participantes, que simularam quedas em diferentes direções enquanto usavam um Apple Watch Series 5. O principal resultado foi que o Apple Watch detectou apenas 14 das 300 quedas realizadas. Isso corresponde a uma sensibilidade de 4,7%, ou seja, o dispositivo falhou em reconhecer a grande maioria das quedas simuladas nesse cenário. Outro ponto importante é que fatores como altura do participante, força do impacto, funcionamento dos membros inferiores e direção da queda podem influenciar a detecção. Isso mostra que algoritmos de detecção de queda dependem fortemente do padrão de movimento e do contexto de uso. A conclusão do estudo é que a detecção de quedas do Apple Watch teve desempenho muito baixo para quedas a partir de cadeira de rodas. Portanto, embora o recurso seja útil em alguns cenários, ele não deve ser considerado plenamente confiável para esse público específico.

**Aplicação Relacionada a Sistemas Embarcados**

A aplicação relacionada a sistemas embarcados está no uso de acelerômetros e algoritmos embarcados para detectar quedas ou movimentos bruscos em dispositivos vestíveis. No contexto do smartwatch com ESP32, o artigo justifica o uso de uma IMU/acelerômetro para monitorar a magnitude da aceleração e gerar alertas locais ou remotos quando houver movimento anormal. Em resumo, o artigo mostra que a detecção de quedas em smartwatches é uma aplicação importante de sistemas embarcados, mas também evidencia que esse tipo de alerta exige calibração cuidadosa para evitar falhas de detecção.

---

## 5. Comparativo com Produtos Similares

A tabela abaixo compara o Apple Watch SE 3 com cinco smartwatches da mesma categoria, abrangendo gerações diferentes e fabricantes distintos, para contextualizar as escolhas de hardware e as limitações do produto de referência.

| Especificação | Apple Watch SE 3 (2024) | Samsung Galaxy Watch 7 (2024) | Garmin Forerunner 265 (2023) | Fitbit Sense 2 (2022) | Apple Watch Series 1 (2015) | Pebble Time (2015) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Processador** | S9 SiP dual-core | Exynos W1000 (5 nm) | Garmin Elevate (proprietário) | Não divulgado | S1 SiP | ARM Cortex-M4 |
| **Sistema Operacional** | watchOS 11 | Wear OS 5 | Garmin OS | Fitbit OS | watchOS 2 | PebbleOS |
| **Tela** | LTPO OLED Retina | Super AMOLED | AMOLED 1,1" | AMOLED 1,58" | Sapphire OLED | e-Paper color 1,25" |
| **Tamanho de tela** | 40 ou 44 mm | 40 ou 44 mm | 42 mm (1,1") | 40 mm | 38 ou 42 mm | 34 × 26 mm |
| **Frequência cardíaca (PPG)** | Sim (contínuo) | Sim (contínuo) | Sim (contínuo) | Sim (contínuo) | Sim (contínuo) | Não (sem PPG) |
| **ECG** | Não | Sim | Não | Sim | Não | Não |
| **SpO₂** | Não | Sim | Sim | Sim | Não | Não |
| **Temperatura corporal** | Não | Sim | Sim | Sim | Não | Não |
| **Acelerômetro / Giroscópio** | Sim / Sim | Sim / Sim | Sim / Sim | Sim / Sim | Sim / Sim | Sim / Não |
| **Detecção de queda** | Sim | Sim | Não | Não | Não | Não |
| **GPS integrado** | Não (usa GPS do iPhone) | Sim | Sim | Não | Não | Não |
| **Rede celular (LTE)** | Não | Versão opcional | Não | Não | Não | Não |
| **Wi-Fi** | 802.11b/g/n/ax | 802.11b/g/n/ac | 802.11b/g/n | Não | 802.11b/g/n | Não |
| **Bluetooth** | 5.3 | 5.3 | 5.3 | 5.0 | 4.0 | 4.0 LE |
| **NFC / Pagamentos** | Sim (Apple Pay) | Sim (Samsung Pay) | Não | Não | Sim (Apple Pay) | Não |
| **Autonomia da bateria** | ~18 h (uso normal) | ~40 h (modo eco) | ~13 dias (relógio) | ~6 dias | ~18 h | ~7 dias |
| **Carregamento** | Magnético (USB-C) | Magnético (USB-C) | USB-C | Magnético USB-C | Magnético (Lightning) | Magnético proprietário |
| **Resistência à água** | WR50 (50 m) | 5 ATM + IP68 | 5 ATM | 5 ATM | IPX7 | 5 ATM |
| **Preço de lançamento (aprox.)** | US$ 249 | US$ 299 | US$ 449 | US$ 299 | US$ 349 | US$ 199 |
| **Foco principal** | Saúde + ecossistema Apple | Saúde + Android | Esporte / corrida | Saúde + bem-estar | Primeiro smartwatch Apple | Notificações + bateria longa |

### Observações sobre o comparativo

O **Apple Watch SE 3** posiciona-se como a opção de entrada do ecossistema Apple, eliminando recursos de custo elevado como ECG, SpO₂, temperatura corporal e GPS integrado, mas mantendo o processador S9 (o mesmo da linha Series 9), detecção de queda e watchOS completo. Em relação ao **Series 1** da própria Apple (2015), a evolução é expressiva em processamento, eficiência energética e sensores, porém o conceito de dependência do iPhone para GPS já estava presente desde o início.

O **Samsung Galaxy Watch 7** é o principal concorrente direto em 2024, oferecendo ECG, SpO₂ e temperatura por preço similar, mas vinculado ao ecossistema Android. O **Garmin Forerunner 265** é voltado a atletas, priorizando GPS de alta precisão e autonomia de até 13 dias, sem recursos de pagamento por aproximação. O **Fitbit Sense 2** destaca-se pelo foco em bem-estar e sono, com autonomia de 6 dias superior à dos concorrentes da Apple. O **Pebble Time** (2015), contemporâneo ao Series 1, é um caso histórico relevante: tela e-Paper com autonomia de 7 dias e SDK aberto atraíram desenvolvedores, mas a ausência de PPG e Wi-Fi limitou as aplicações de saúde.

Para o protótipo ESP32, o comparativo reforça que o conjunto mínimo viável de um smartwatch IoT de saúde — sensor PPG, acelerômetro e conectividade sem fio — está presente desde as primeiras gerações do Apple Watch e pode ser reproduzido com hardware acessível, como demonstrado neste trabalho.

---

## 6. Referências

> Fundamentos de Sistemas Embarcados (FGA-UnB). **Lista de Sensores — Trabalho 2, 2026-1**. Disponível em: [https://gitlab.com/fse_fga/trabalhos-2026_1/trabalho-2-2026-1/-/blob/main/Lista_de_Sensores.md](https://gitlab.com/fse_fga/trabalhos-2026_1/trabalho-2-2026-1/-/blob/main/Lista_de_Sensores.md). Acesso em: 12 jun. 2026.

> APPLE. **Apple Watch SE**. Disponível em: [https://www.apple.com/br/apple-watch-se/](https://www.apple.com/br/apple-watch-se/). Acesso em: 12 jun. 2026.

> THINGSBOARD. **Documentation — MQTT API**. Disponível em: [https://thingsboard.io/docs/reference/mqtt-api/](https://thingsboard.io/docs/reference/mqtt-api/). Acesso em: 12 jun. 2026.

> CROW, B. P. et al. IEEE 802.11 wireless local area networks. **IEEE Communications Magazine**, v. 35, n. 9, p. 116–126, 1997. DOI: 10.1109/35.620533.

> BAÑOS-GONZALEZ, V. et al. IEEE 802.11ah: a technology to face the IoT challenge. **Sensors**, v. 16, n. 11, artigo 1960, 2016. DOI: 10.3390/s16111960.

> QUINCOZES, S. E.; EMILIO, T.; KAZIENKO, J. F. MQTT Protocol: Fundamentals, Tools and Future Directions. **IEEE Latin America Transactions**, v. 17, n. 9, p. 1439–1448, 2019. DOI: 10.1109/TLA.2019.8931137.

> AKSHATHA, P. S.; DILIP KUMAR, S. M.; VENUGOPAL, K. R. MQTT Implementations, Open Issues, and Challenges: A Detailed Comparison and Survey. **International Journal of Sensors, Wireless Communications and Control**, v. 12, n. 8, p. 553–576, 2022. DOI: 10.2174/2210327913666221216152446.

> ALLEN, John. Photoplethysmography and its application in clinical physiological measurement. **Physiological Measurement**, v. 28, n. 3, p. R1–R39, 2007. DOI: 10.1088/0967-3334/28/3/R01.

> PEREZ, Marco V. et al. Large-scale assessment of a smartwatch to identify atrial fibrillation. **The New England Journal of Medicine**, v. 381, n. 20, p. 1909–1917, 2019. DOI: 10.1056/NEJMoa1901183.

> FALTER, Maarten et al. Accuracy of Apple Watch measurements for heart rate and energy expenditure in patients with cardiovascular disease: cross-sectional study. **JMIR mHealth and uHealth**, v. 7, n. 3, e11889, 2019. DOI: 10.2196/11889.

> KHUSHHAL, Alaa et al. Validity and reliability of the Apple Watch for measuring heart rate during exercise. **Sports Medicine International Open**, v. 1, n. 6, p. E206–E211, 2017. DOI: 10.1055/s-0043-120195.

> ABOU, Libak et al. Sensitivity of Apple Watch fall detection feature among wheelchair users. **Assistive Technology**, v. 34, n. 5, p. 619–625, 2022. DOI: 10.1080/10400435.2021.1923087.