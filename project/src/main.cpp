#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"

// ==========================================
//           CREDENCIAIS E CONFIGURAÇÕES
// ==========================================
#define WIFI_SSID "Plus Energy"
#define WIFI_PASS "PlusEnergy@123"
#define APP_KEY "6773556b-5362-4987-ba9d-e45caf81fd7c"
#define APP_SECRET "6b793176-5973-456d-8678-441090772d4a-6b6e6dc8-e803-483e-9b95-6f7d0a93e2ca"
#define PULSEIRA_ID "6a2c2839c93c9fd0e00d3554"

#define I2C_ADDR_MPU6050 0x68
#define PINO_BOTAO 4
#define I2C1_SDA 21 // Exclusivo MAX30100 (Core 0)
#define I2C1_SCL 22 // Exclusivo MAX30100 (Core 0)
#define I2C2_SDA 32 // Exclusivo MPU6050 (Core 1)
#define I2C2_SCL 33 // Exclusivo MPU6050 (Core 1)

Adafruit_MPU6050 mpu;
PulseOximeter pox;

// Variáveis voláteis globais para transferência segura de dados entre núcleos
volatile float bpmGlobal = 0.0;
volatile uint8_t spo2Global = 0;

// Timers do Core 1
unsigned long tProcessamentoSensores = 0;
unsigned long tFiltroJanelaNuvem = 0;
unsigned long tDebugTelemetria = 0;
unsigned long tUltimoAlertaEnviado = 0;

const int INTERVALO_SENSORES = 100;
const int INTERVALO_DEBUG = 1000;
const int COOLDOWN_ALERTAS = 10000;
TaskHandle_t task_30102 = NULL;
bool alarmeAguardandoResetNuvem = false;

static void debug_print(void);
static float module(sensors_vec_t vec);

// currently unused fallback function
void onBeatDetected()
{
}

// another unused callback
bool onPowerState(const String &deviceId, bool &state)
{
	return true;
}

// ========================================================
// TAREFA EXCLUSIVA DO CORE 0 - AMOSTRAGEM DO MAX30100
// ========================================================
void tarefaMAX30100(void *pvParameters)
{
	Serial.print("[CORE 0] Inicializando MAX30100 no núcleo: ");
	Serial.println(xPortGetCoreID());

	Wire.begin(I2C1_SDA, I2C1_SCL);
	if (!pox.begin()) {
		Serial.println("[ERRO CRÍTICO CORE 0] Falha no MAX30100");
		while (1)
			;
	}

	// Retorna para a configuração de corrente idêntica ao teste isolado de sucesso
	pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
	pox.setOnBeatDetectedCallback(onBeatDetected);

	unsigned long tEscritaGlobal = 0;

	while (1) {
		pox.update(); // Atualiza o buffer FIFO continuamente sem nenhuma concorrência

		// Transfere os dados filtrados para as variáveis globais a cada 500ms
		if (millis() - tEscritaGlobal > 500) {
			bpmGlobal = pox.getHeartRate();
			spo2Global = pox.getSpO2();
			tEscritaGlobal = millis();
		}

		// Libera 1ms para o scheduler do FreeRTOS não travar o watchdog do Core 0
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

// ==========================================
//          SETUP (EXECUTA NO CORE 1)
// ==========================================
void setup()
{
	Serial.begin(115200);
	pinMode(PINO_BOTAO, INPUT_PULLUP);

	// 1. Cria a tarefa assíncrona no CORE 0 antes de ligar o WiFi
	xTaskCreatePinnedToCore(tarefaMAX30100, "TaskMAX", 4096, NULL, 1, &task_30102, 0);

	// Aguarda a estabilização do Core 0
	delay(500);

	// 2. Inicializa o MPU6050 no Barramento Secundário (Core 1)
	Wire1.begin(I2C2_SDA, I2C2_SCL);
	if (!mpu.begin(0x68, &Wire1)) {
		Serial.println("[ERRO CORE 1] MPU6050 não encontrado!");
		while (1)
			;
	}
	mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

	// 3. Inicializa Conexões de Rede (Core 1)
	Serial.print("[REDE] Conectando WiFi");
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println(" OK!");

	// 4. Inicializa Link IoT (Core 1)
	SinricProSwitch &mySwitch = SinricPro[PULSEIRA_ID];
	mySwitch.onPowerState(onPowerState);
	SinricPro.begin(APP_KEY, APP_SECRET);
}

// ==========================================
//        LOOP PRINCIPAL (EXECUTA NO CORE 1)
// ==========================================
void loop()
{
	SinricPro.handle(); // Processa a rede no Core 1 sem atrasar o Core 0

	// Máquina de Estados do Alarme
	if (alarmeAguardandoResetNuvem && (millis() - tFiltroJanelaNuvem > 3000)) {
		SinricProSwitch &mySwitch = SinricPro[PULSEIRA_ID];
		mySwitch.sendPowerStateEvent(false);
		alarmeAguardandoResetNuvem = false;
	}

	// Bloco de Varredura e Gatilhos (100ms)
	if (millis() - tProcessamentoSensores > INTERVALO_SENSORES) {
		bool dispararEmergencia = false;
		String causaAlerta = "";

		if (digitalRead(PINO_BOTAO) == LOW) {
			dispararEmergencia = true;
			causaAlerta = "Botão de Pânico Acionado!";
		}

		sensors_event_t acelaracao, giroscopio, temperatura;
		mpu.getEvent(&acelaracao, &giroscopio, &temperatura);
		float magnitudeG = module(acelaracao.acceleration);

		if (magnitudeG > 25.0) {
			dispararEmergencia = true;
			causaAlerta = "Queda Brusca Detectada!";
		}

		// Lendo as variáveis globais preenchidas pelo Core 0 de forma segura
		float bpmLocal = bpmGlobal;
		uint8_t spo2Local = spo2Global;

		if (bpmLocal > 0) {
			if (bpmLocal < 40.0 || bpmLocal > 130.0) {
				dispararEmergencia = true;
				causaAlerta = "Frequência Cardíaca Crítica!";
			} else if (spo2Local > 0 && spo2Local < 90) {
				dispararEmergencia = true;
				causaAlerta = "Hipóxia Crítica!";
			}
		}

		if (dispararEmergencia && (millis() - tUltimoAlertaEnviado > COOLDOWN_ALERTAS)) {
			Serial.println("\n>>> [ALERTA] DISPARANDO EVENTO: " + causaAlerta);
			SinricProSwitch &mySwitch = SinricPro[PULSEIRA_ID];
			mySwitch.sendPowerStateEvent(true);
			mySwitch.sendPushNotification(causaAlerta);

			tFiltroJanelaNuvem = millis();
			alarmeAguardandoResetNuvem = true;
			tUltimoAlertaEnviado = millis();
		}

		tProcessamentoSensores = millis();
	}

	// Saída de Telemetria (1000ms)
	debug_print();
}

static void debug_print(void)
{
	if (millis() - tDebugTelemetria > INTERVALO_DEBUG) {
		sensors_event_t a_debug, g_debug, t_debug;
		mpu.getEvent(&a_debug, &g_debug, &t_debug);

		float gTotal = module(a_debug.acceleration);

		Serial.print("[DATA] Magnitude G: ");
		Serial.print(gTotal);
		Serial.print(" m/s² | FC: ");
		Serial.print(bpmGlobal);
		Serial.print(" bpm | SpO2: ");
		Serial.print(spo2Global);
		Serial.println("%");

		tDebugTelemetria = millis();
	}
}

static float module(sensors_vec_t vec)
{
	return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
}
