#include "freertos/portmacro.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "credentials.h"

#define BAUD_RATE 115200
#define I2C_ADDR_MPU6050 0x68
#define BTN_PIN 4
#define I2C1_SDA 21
#define I2C1_SCL 22
#define I2C2_SDA 32
#define I2C2_SCL 33
#define INTERVALO_SENSORES 100
#define INTERVALO_DEBUG 1000
#define COOLDOWN_ALERTAS 10000

#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

Adafruit_MPU6050 mpu;
PulseOximeter pox;

enum ALERT_CODE : uint8_t { ALERT_BTN, ALERT_COLLISION, ALERT_HEARTRATE, ALERT_O2, ALERT_NONE };
String alerts[] = { "Botão de Pânico Acionado!", "Queda Brusca Detectada!",
		    "Frequência Cardíaca Crítica!", "Hipóxia Crítica!" };

// vars for task data sharing
volatile float bpm_global = 0.0;
volatile uint8_t spo2_global = 0;

static float magnitude = 0;

// Timers
uint32_t t_tick_sensors = 0;
uint32_t t_activated_alarm = 0;
uint32_t t_telemtry = 0;
uint32_t t_last_alert = 0;

bool disable_alarm = false;

static void print_data(void);
static float module(sensors_vec_t vec);
static void onBeatDetected(void);
static bool onPowerState(const String &deviceId, bool &state);

// task for reading MAX30100 data on parallel
void max30100_task(void *pvParameters)
{
	DEBUG_PRINT("[CORE 0] Inicializando MAX30100 no núcleo: ");
	DEBUG_PRINTLN(xPortGetCoreID());

	Wire.begin(I2C1_SDA, I2C1_SCL);
	DEBUG_PRINTLN("[OXM] Configurando I2C");
	while (!pox.begin()) {
		DEBUG_PRINTLN("[OXM] MAX30100 não encontrado, tentando novamente!");
		delay(1000);
	}
	DEBUG_PRINTLN("[OXM] OK!");

	// use tested current
	pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
	pox.setOnBeatDetectedCallback(onBeatDetected);

	uint32_t t_update_vars = 0;

	while (1) {
		pox.update();

		if (millis() - t_update_vars > 500) {
			bpm_global = pox.getHeartRate();
			spo2_global = pox.getSpO2();
			t_update_vars = millis();
		}

		//keep watchdog alive
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

// SETUP (CORE 1)
void setup()
{
	Serial.begin(BAUD_RATE);
	pinMode(BTN_PIN, INPUT_PULLUP);

	// Oximeter task
	BaseType_t task = xTaskCreatePinnedToCore(max30100_task, "TaskMAX", 4096, NULL, 1, NULL, 0);
	configASSERT(task == pdPASS);

	// Mpu6050
	Wire1.begin(I2C2_SDA, I2C2_SCL);
	DEBUG_PRINT("[MPU] Configurando I2C");
	while (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire1)) {
		DEBUG_PRINTLN("[MPU] MPU6050 não encontrado, tentando novamente!");
		delay(1000);
	}
	DEBUG_PRINTLN("[MPU] OK!");
	mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

	//Wifi
	DEBUG_PRINT("[WIFI] Conectando WiFi");
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		DEBUG_PRINT(".");
	}
	DEBUG_PRINTLN("[WIFI] OK!");

	// SinricPro
	SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
	mySwitch.onPowerState(onPowerState);
	SinricPro.begin(APP_KEY, APP_SECRET);
}

void loop()
{
	SinricPro.handle();

	// desativar o alarme depois de 3 segundos
	if (disable_alarm && (millis() - t_activated_alarm > 3000)) {
		SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
		mySwitch.sendPowerStateEvent(false);
		disable_alarm = false;
	}

	//sensors logic loop (100ms)
	if (millis() - t_tick_sensors > INTERVALO_SENSORES) {
		uint8_t alert_id = ALERT_NONE;

		// check for emergency btn
		if (digitalRead(BTN_PIN) == LOW) {
			alert_id = ALERT_BTN;
		}

		// check for collisions
		sensors_event_t acell_ev, gyro_ev, temp_ev;
		mpu.getEvent(&acell_ev, &gyro_ev, &temp_ev);
		magnitude = module(acell_ev.acceleration);
		if (magnitude > 25.0) {
			alert_id = ALERT_COLLISION;
		}

		// Read values shared from 30102 task
		float bpm_local = bpm_global;
		uint8_t spo2_local = spo2_global;

		// check oximeter and heartrate
		if (bpm_local > 0) {
			if (bpm_local < 40.0 || bpm_local > 130.0) {
				alert_id = ALERT_HEARTRATE;
			} else if (spo2_local > 0 && spo2_local < 90) {
				alert_id = ALERT_O2;
			}
		}

		// handle alert
		if (alert_id != ALERT_NONE && (millis() - t_last_alert > COOLDOWN_ALERTAS)) {
			DEBUG_PRINTLN("\n>>> [ALERTA] DISPARANDO EVENTO: " + alerts[alert_id]);
			SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
			mySwitch.sendPowerStateEvent(true);
			mySwitch.sendPushNotification(alerts[alert_id]);

			t_activated_alarm = millis();
			disable_alarm = true;
			t_last_alert = millis();
		}

		t_tick_sensors = millis();
	}

	//debug_output (1000ms)
	print_data();
}

static void print_data(void)
{
#ifdef DEBUG
	if (millis() - t_telemtry > INTERVALO_DEBUG) {
		DEBUG_PRINT("[DATA] Magnitude G: ");
		DEBUG_PRINT(magnitude);
		DEBUG_PRINT(" m/s² | FC: ");
		DEBUG_PRINT(bpm_global);
		DEBUG_PRINT(" bpm | SpO2: ");
		DEBUG_PRINT(spo2_global);
		DEBUG_PRINTLN("%");
		t_telemtry = millis();
	}
#endif
}

static float module(sensors_vec_t vec)
{
	return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
}

// currently unused fallback function
static void onBeatDetected(void)
{
	return;
}

// another unused callback
bool onPowerState(const String &deviceId, bool &state)
{
	return true;
}
