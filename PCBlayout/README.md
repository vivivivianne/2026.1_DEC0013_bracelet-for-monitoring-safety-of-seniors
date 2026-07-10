# Monitor de Saúde Assistivo IoT: Telemetria e Detecção de Quedas

**Especificação de Hardware e Relatório Técnico de Engenharia**

---

## 1. Visão Geral e Escopo do Projeto

Este projeto documenta o desenvolvimento de um dispositivo vestível (*wearable*) miniaturizado (PCB de 40 mm × 40 mm), projetado para o monitoramento assistivo contínuo de idosos. O sistema realiza a telemetria de sinais vitais (frequência cardíaca e saturação de oxigênio), identifica quedas bruscas de forma proativa utilizando uma Unidade de Medida Inercial (IMU) e disponibiliza um botão físico de emergência (SOS). 

**Aviso de Uso Pretendido:** Este design é um protótipo educacional e de validação de conceito para sistemas embarcados e IoT. **Não se trata de um dispositivo médico certificado.** Qualquer alegação clínica ou diagnóstica exige verificação independente e certificação regulatória (ex: ANVISA/FDA). Assume-se que a bateria LiPo utilizada possua seu próprio circuito de proteção (PCM) integrado.

---

## 2. Arquitetura do Sistema e Fluxo de Dados

A topologia foi desenhada para garantir isolamento de ruído, eficiência na distribuição de energia e comunicação assíncrona via infraestrutura em nuvem (SinricPro).

```mermaid
flowchart TD
  USB[Porta USB-C 5V] --> CHG[Carregador LiPo TP4056]
  CHG --> BAT[Conector LIPO_BATT / Nó BAT]
  BAT --> LDO[Regulador LDO AP2112K-3.3]
  LDO --> V3V3[Barramento Principal 3V3]
  V3V3 --> ESP[Módulo ESP32-WROOM-32E]
  V3V3 --> MAXHDR[Slot MAX30102]
  V3V3 --> MPUHDR[Slot MPU6050]
  ESP --> UART[Header UART 1x05]
  ESP --> MAXHDR
  ESP --> MPUHDR
  BTN[Botão de Pânico] --> ESP
  ```
  ---

  ## 3. Subsistemas de Hardware

### 3.1. Processamento e RF (Compute & RF)
* **Microcontrolador Central:** Módulo ESP32-WROOM-32E alimentado em 3.3V.
* **Circuito de Boot:** O pino `EN` utiliza um resistor de *pull-up* de 10 kΩ para 3.3V e um capacitor de 1 µF para o GND para garantir estabilidade na inicialização. O pino `IO0` está exposto no Header UART para forçar o modo de gravação.
* **Restrição de RF:** A antena de PCB deve ultrapassar a borda da placa. É exigida uma zona de exclusão (*Keep-out zone*) absoluta sob a antena, livre de pistas de cobre, componentes ou planos de terra em todas as camadas.

### 3.2. Gerenciamento de Energia (Power)
* **Entrada:** Receptáculo USB-C (Sink-only) com resistores *pull-down* de 5.1 kΩ nos pinos CC1 e CC2 para negociação de 5V.
* **Carregador LiPo:** CI TP4056 linear. Para garantir a segurança térmica de uma célula LiPo de baixa capacidade (wearable), o resistor do pino `PROG` foi dimensionado para 10 kΩ, limitando a corrente de carga nominal a aproximadamente 130 mA.
* **Regulação:** LDO AP2112K-3.3, gerando o barramento de 3.3V com capacidade de 600 mA para suprir os transientes do Wi-Fi.

### 3.3. Sensores e Módulos Secundários
Para simplificar a montagem do protótipo, os sensores utilizam *Daughterboards* através de conectores na placa principal:
* **Interface I2C 0:** Oxímetro MAX30102 alocado nos pinos GPIO21 (SDA) e GPIO22 (SCL).
* **Interface I2C 1:** Acelerômetro MPU6050 alocado nos pinos GPIO32 (SDA) e GPIO33 (SCL). O pino AD0 está aterrado (Endereço 0x68).
* *Nota:* O isolamento físico em dois barramentos I2C impede a queda da resistência equivalente gerada pelos *pull-ups* nativos dos módulos, evitando colisão de dados.

---

## 4. Interfaces e Mapeamento de Pinos

| Interface | Conector Físico | Conexões Elétricas / GPIOs |
| :--- | :--- | :--- |
| **Carga USB-C** | Receptáculo USB-C SMD | VBUS, GND, CC1, CC2 |
| **Bateria LiPo** | SMD Pad genérico (2 pinos) | BAT, GND |
| **Gravação UART** | Header Macho 1×05 (2.54mm) | 3V3, GND, TXD0, RXD0, IO0 |
| **Módulo MAX30102** | Header Fêmea 1×07 (Bottom Layer) | VIN, GND, SCL (22), SDA (21), INT (Flutuante) |
| **Módulo MPU6050** | Header Fêmea 1×08 (Top Layer) | VCC, GND, SCL (33), SDA (32), AD0 (GND) |
| **Botão de Pânico** | Pushbutton Tactil SMD | GPIO4, GND (Pull-up interno via firmware) |

---

## 5. Orçamento de Potência (Power Budget)

A autonomia de operação dependerá da capacidade da célula LiPo selecionada. A tabela abaixo reflete as estimativas de carga do sistema:

| Barramento | Origem | Cargas Principais | Consumo Típico | Consumo de Pico (Estimado) |
| :--- | :--- | :--- | :---: | :---: |
| **VBUS (5V)** | USB-C | Entrada TP4056 | ~ 130 mA (Em carga) | 130 mA (Limitado por HW) |
| **BAT (LiPo)** | TP4056 / Célula | Entrada LDO | Igual à carga de 3.3V | > 500 mA (Rajadas de RF) |
| **3V3** | LDO AP2112K-3.3 | ESP32, MAX30102, MPU6050 | 150 a 260 mA | 500 a 650 mA (Transiente) |

*Risco Mapeado:* A queda de tensão (Dropout Voltage) do regulador AP2112K-3.3 pode limitar o tempo de operação (Runtime) à medida que a tensão da bateria LiPo se aproxima de 3.4V.

---

## 6. Diretrizes de Layout da PCB (Design Constraints)

* **Formato:** PCB de 40 mm × 40 mm estritos, com cantos arredondados, em 2 camadas (Top e Bottom).
* **Posicionamento:** Header do MAX30102 obrigatoriamente na camada inferior (Bottom Layer) voltado para o pulso do usuário. Demais componentes na camada superior (Top Layer).
* **Dimensionamento de Pistas (IPC-2221):** Largura mínima de 0.5 mm para pistas de potência (VBUS, BAT, 3V3, GND) visando suportar correntes de pico transientes. Pistas de sinal (I2C, UART) operam a 0.25 mm.
* **Malha de Terra:** Preenchimento completo das áreas ociosas com polígonos de cobre conectados ao GND (Copper Pour) em ambas as camadas, exceto na zona de exclusão da antena (RF Keep-out).

---

## 8. Lista de Materiais (BOM - Bill of Materials) da PCB

Abaixo está o detalhamento dos componentes de montagem em superfície (SMD) e conectores (THT) exigidos para a fabricação e montagem da placa de circuito impresso (PCBA) de 40x40mm. Os valores são estimativas para a montagem de um protótipo de validação (lote unitário).

| Designador | Componente / Especificação | Encapsulamento (Footprint) | Qtd | Custo Est. (R$) |
| :--- | :--- | :--- | :---: | :---: |
| **U1** | Microcontrolador ESP32-WROOM-32E | Módulo SMD (Espressif 38-pin) | 1 | 25,00 |
| **U2** | Carregador LiPo TP4056 | SOP-8 | 1 | 2,00 |
| **U3** | Regulador LDO AP2112K-3.3 (600mA) | SOT-23-5 | 1 | 3,00 |
| **J1** | Receptáculo USB Type-C (Power Only) | SMD 16-pin ou 6-pin | 1 | 3,00 |
| **J2** | Conector Bateria JST PH 2.0mm | SMD Vertical (2 pinos) | 1 | 1,50 |
| **J3** | Header Fêmea (Interface MAX30102) | THT 1x07 (Pitch 2.54mm) | 1 | 1,00 |
| **J4** | Header Fêmea (Interface MPU6050) | THT 1x08 (Pitch 2.54mm) | 1 | 1,00 |
| **J5** | Header Macho (Gravação UART/Boot) | THT 1x05 (Pitch 2.54mm) | 1 | 1,00 |
| **SW1** | Pushbutton Táctil (Botão SOS) | SMD 4x4mm ou 6x6mm | 1 | 0,50 |
| **R1, R2** | Resistor 10 kΩ (Pull-up EN e PROG 130mA) | SMD 0603 ou 0805 | 2 | 0,20 |
| **R3, R4** | Resistor 5.1 kΩ (Pull-down CC1/CC2 USB) | SMD 0603 ou 0805 | 2 | 0,20 |
| **C1** | Capacitor Cerâmico 1 µF (Circuito Lógico EN) | SMD 0603 ou 0805 | 1 | 0,10 |
| **C2 a C5**| Capacitor Cerâmico 10 µF (Filtros LDO e TP4056)| SMD 0805 ou 1206 | 4 | 0,40 |
| **-** | Fabricação da PCB 40x40mm FR4 | 2 Camadas, 1.6mm ou 1.2mm | 1 | 25,00 |
| | | | **Subtotal Placa:** | **R$ 63,90** |

### 8.1. Periféricos Modulares Externos à PCB Principal
Para fins de validação do conceito e viabilidade de soldagem manual, os sensores óticos e inerciais foram mantidos em suas *Daughterboards* originais e acoplados aos conectores J3 e J4 da placa principal.

| Item | Especificação | Conexão à PCB | Custo Est. (R$) |
| :--- | :--- | :--- | :---: |
| **Módulo Oximetria** | Placa MAX30102 (Endereço I2C fixo) | Encaixe no Header J3 (Bottom) | 35,00 |
| **Módulo IMU** | Placa MPU6050 (Endereço I2C 0x68) | Encaixe no Header J4 (Top) | 20,00 |
| **Célula de Energia**| Bateria LiPo 3.7V (~250mAh) com circuito PCM | Plug JST (J2) | 35,00 |
| | | **Custo Total do Protótipo:**| **~ R$ 153,90** |

---

## 9. Instruções de Compilação e Instalação (Setup)

O firmware foi desenvolvido utilizando o ecossistema **PlatformIO**. A utilização da Arduino IDE padrão não é recomendada devido à gestão complexa de dependências do FreeRTOS e das bibliotecas de sensores.

### 9.1. Pré-requisitos
1. Instalar o Visual Studio Code (VSCode) ou a IDE Zed.
2. Instalar a extensão **PlatformIO IDE**.
3. Possuir um adaptador USB-Serial (FTDI) configurado para 3.3V.

### 9.2. Configuração do Ambiente (`platformio.ini`)
O arquivo de configuração deve conter a declaração estrita das bibliotecas utilizadas para evitar incompatibilidades de versão:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    adafruit/Adafruit MPU6050 @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.9
    oxullo/MAX30100lib @ ^1.2.1
    sinricpro/SinricPro @ ^3.2.0
```
---

### 9.3. Credenciais e Variáveis de Ambiente

Antes de iniciar o processo de compilação, é estritamente necessário configurar as chaves de autenticação da infraestrutura em nuvem e as credenciais de rede local. Abra o arquivo `src/main.cpp` e edite as definições no cabeçalho do código:

```cpp
#define WIFI_SSID         "INSERIR_SSID_DA_REDE"
#define WIFI_PASS         "INSERIR_SENHA_DA_REDE"
#define APP_KEY           "INSERIR_APP_KEY_DO_SINRICPRO"
#define APP_SECRET        "INSERIR_APP_SECRET_DO_SINRICPRO"
#define PULSEIRA_ID       "INSERIR_DEVICE_ID_DO_SWITCH_VIRTUAL"
```

### 9.4. Procedimento de Gravação Física (Upload)

Dado que a placa de circuito impresso foi desenvolvida utilizando o módulo ESP32-WROOM-32E puro, sem a presença de um circuito conversor USB-Serial (como o CP2102 ou CH340) integrado à PCB para minimizar a dimensão física e o consumo de energia, a gravação do firmware exige a utilização de um adaptador FTDI externo configurado rigorosamente para a tensão lógica de 3.3V.

1. **Conexão de Dados:** Conecte os pinos do adaptador FTDI ao conector J5 (Header UART) da placa efetuando o cruzamento dos sinais lógicos: o pino TX do FTDI deve ser conectado ao RXD0 da placa, e o pino RX do FTDI ao TXD0 da placa. Conecte os pinos 3V3 e GND aos seus respectivos correspondentes para estabelecer a referência elétrica comum.
2. **Ativação do Modo de Download:** O microcontrolador ESP32 requer o aterramento lógico do pino de boot durante o ciclo de inicialização para permitir a escrita de novo firmware na memória flash. Conecte um jumper provisório entre o pino IO0 (exposto no conector J5) e qualquer ponto de GND da placa.
3. **Alimentação e Compilação:** Com o circuito energizado e os pinos conectados, execute o comando de Upload através do painel integrado do PlatformIO.
4. **Finalização de Boot:** Aguarde a confirmação de escrita bem-sucedida no terminal (frequentemente indicada pela mensagem `Leaving... Hard resetting via RTS pin...`). Desconecte imediatamente o jumper do pino IO0 e reinicie a alimentação da placa (ciclo de power-off/power-on) para que a aplicação seja executada normalmente a partir da memória flash.
