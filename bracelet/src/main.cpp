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
#include "types.h"

#define BAUD_RATE 115200
#define I2C_ADDR_MPU6050 0x68
#define BTN_PIN 4
#define I2C1_SDA 21
#define I2C1_SCL 22
#define I2C2_SDA 32
#define I2C2_SCL 33
#define SENSORS_INTERVAL 1
#define OXM_INTERVAL 400
#define DEBUG_INTERVAL 100
#define ALERT_COOLDOWN 10000

#ifdef DEBUG

#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
// smaller thresholds for testing!
#define ACCEL_DANGER_THRESHOLD 14
#define ACCEL_DIFF_THRESHOLD 1

#else
#define DEBUG_PRINT(fmt, ...) ((void)0)

#define ACCEL_DANGER_THRESHOLD 16
#define ACCEL_DIFF_THRESHOLD 2

#endif

Adafruit_MPU6050 mpu;
PulseOximeter pox;

// enum ERROR_TYPES { ERR_THREAD, ERR_OXM, ERR_MPU6050, ERR_OTHER };
// enum DEBUG { };
// String debug_strings[];

enum ALERT_CODE : u8 { ALERT_BTN, ALERT_COLLISION, ALERT_HEARTRATE, ALERT_O2, ALERT_NONE };
static enum ALERT_CODE alert_id = ALERT_NONE;

#ifdef LANG_BR
String alerts[] = { "Botão de Pânico Acionado!", "Queda Brusca Detectada!",
		    "Frequência Cardíaca Crítica!", "Hipóxia Crítica!" };
#else
String alerts[] = { "Panic Button Activated!", "Sudden Drop Detected!", "Critical heart rate!",
		    "Critical Hipoxia!" };
#endif

// vars for task data sharing
volatile f32 bpm_global = 0.0;
volatile u8 spo2_global = 0;

static f32 magnitude = 0;

// Timers
u32 t_tick_sensors = 0;
u32 t_activated_alarm = 0;
u32 t_telemtry = 0;
u32 t_last_alert = 0;
u32 t_modules_update = 0;

static void print_data(void);
static f32 module(sensors_vec_t vec);
static void onBeatDetected(void);
static bool onPowerState(const String &deviceId, bool &state);

#define MODULES_LEN 15
#define MODULES_UPDATE_INTERVAL 100
static f32 modules_avg = 0;
static f32 modules[MODULES_LEN] = {};
static u8 modules_i = 0;
static void buffer_update(f32 magnitude);
static f32 buffer_get_avg(void);
static bool detect_collision(f32 magnitude);
static enum ALERT_CODE detect_bpm_spo(f32 bpm, u8 spo2);

// task for reading MAX30100 data on parallel
void max30100_task(void *pvParameters)
{
	DEBUG_PRINT("[OXM] Initializing MAX30102 Task : %d\n", xPortGetCoreID());
	delay(100);

	DEBUG_PRINT("[OXM] Configuring I2C\n");
	Wire.begin(I2C1_SDA, I2C1_SCL);

	while (!pox.begin()) {
		DEBUG_PRINT("[OXM] MAX30100 not found, trying again!\n");
		delay(1000);
	}
	DEBUG_PRINT("[OXM] OK!\n");

	// use tested current
	pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
	pox.setOnBeatDetectedCallback(onBeatDetected);

	u32 t_update_vars = 0;
	while (1) {
		pox.update();

		if (millis() - t_update_vars > OXM_INTERVAL) {
			bpm_global = pox.getHeartRate();
			spo2_global = pox.getSpO2();
			t_update_vars = millis();
		}
	}
	//keep watchdog alive
	vTaskDelay(1 / portTICK_PERIOD_MS);
}

// SETUP (CORE 1)
void setup()
{
	Serial.begin(BAUD_RATE);
	pinMode(BTN_PIN, INPUT_PULLUP);

	// Oximeter task
#ifndef DISABLE_OXM
	BaseType_t task = xTaskCreatePinnedToCore(max30100_task, "TaskMAX", 4096, NULL, 1, NULL, 0);
	configASSERT(task == pdPASS);
#endif

	// Mpu6050
	Wire1.begin(I2C2_SDA, I2C2_SCL);
	DEBUG_PRINT("[MPU] Configuring I2C\n");
	while (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire1)) {
		DEBUG_PRINT("[MPU] MPU6050 Not found, Trying again!\n");
		delay(1000);
	}
	DEBUG_PRINT("[MPU] OK!\n");
	mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

	//Wifi
	DEBUG_PRINT("[WIFI] Conecting to wifi\n");
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		DEBUG_PRINT(".\n");
	}
	DEBUG_PRINT("[WIFI] OK!\n");

	// SinricPro
	SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
	mySwitch.onPowerState(onPowerState);
	SinricPro.begin(APP_KEY, APP_SECRET);
}

void loop()
{
	SinricPro.handle();

	// desativar o alarme depois de 3 segundos
	if (alert_id != ALERT_NONE && (millis() - t_activated_alarm > 3000)) {
		SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
		mySwitch.sendPowerStateEvent(false);
		alert_id = ALERT_NONE;
	}

	//sensors logic loop
	if ((millis() - t_tick_sensors > SENSORS_INTERVAL && alert_id == ALERT_NONE)) {
		// check for emergency btn
		if (digitalRead(BTN_PIN) == LOW) {
			alert_id = ALERT_BTN;
		}

		// check for collisions
		sensors_event_t acell_ev, gyro_ev, temp_ev;
		mpu.getEvent(&acell_ev, &gyro_ev, &temp_ev);
		magnitude = module(acell_ev.acceleration);
		if (detect_collision(magnitude)) {
			alert_id = ALERT_COLLISION;
		}

		// Read values shared from 30102 task
		f32 bpm_local = bpm_global;
		u8 spo2_local = spo2_global;

		// check oximeter and heartrate
		alert_id = detect_bpm_spo(bpm_local, spo2_local);

		// handle alert
		if (alert_id != ALERT_NONE) {
			if (millis() - t_last_alert > ALERT_COOLDOWN) {
				DEBUG_PRINT("\n [ALERT] TRIGGERING EVENT : %s",
					    alerts[alert_id].c_str());
				SinricProSwitch &mySwitch = SinricPro[BRACELET_ID];
				mySwitch.sendPowerStateEvent(true);
				mySwitch.sendPushNotification(alerts[alert_id]);
				// reset modules array
				memset(modules, 0, sizeof(f32) * MODULES_LEN);
				t_activated_alarm = millis();
				t_last_alert = millis();
			} else {
				// this will print a lot of stuff
				// it's interesting to see how many of the readings
				// would've triggered the alarm!
				DEBUG_PRINT(
					"A recent alert was already triggered, waiting cooldown.\n");
				alert_id = ALERT_NONE;
			}
		}
		t_tick_sensors = millis();
	}
	if (millis() - t_modules_update > MODULES_UPDATE_INTERVAL && alert_id == ALERT_NONE) {
		t_modules_update = millis();
		buffer_update(magnitude);
	}
	//debug_output (1000ms)
	print_data();
}

static void print_data(void)
{
#ifdef DEBUG
	if (millis() - t_telemtry > DEBUG_INTERVAL && alert_id == ALERT_NONE) {
		DEBUG_PRINT(
			"[DATA] Magnitude: %.2f m/s² | Avg: %.2f m/s² | Bpm: %.2f bpm | SpO2: %d %% \n",
			magnitude, modules_avg, bpm_global, spo2_global);
		t_telemtry = millis();
	}
#endif
}

static f32 module(sensors_vec_t vec)
{
	return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
}

// currently unused callback function
static void onBeatDetected(void)
{
	return;
}

// another unused callback
bool onPowerState(const String &deviceId, bool &state)
{
	return true;
}

static void buffer_update(f32 value)
{
	if (modules_i < MODULES_LEN) {
		modules[modules_i] = value;
		modules_i++;
	} else {
		modules_i = 0;
	}
	modules_avg = buffer_get_avg();
}

static f32 buffer_get_avg(void)
{
	f32 sum = 0.f;
	for (u8 i = 0; i < MODULES_LEN; i++) {
		sum += modules[i];
	}
	return sum / MODULES_LEN;
};

static bool detect_collision(f32 magnitude)
{
	bool dangerous_spd = (modules_avg > ACCEL_DANGER_THRESHOLD);
	f32 diff = (magnitude - modules_avg);
	bool crash = ((diff < 0) && (abs(diff) > ACCEL_DIFF_THRESHOLD));

	if (dangerous_spd && crash) {
		DEBUG_PRINT("[COLLISION DETECTED] - DIFF: %f", diff);
		return true;
	}
	return false;
}

static enum ALERT_CODE detect_bpm_spo(f32 bpm, u8 spo2)
{
#ifndef DISABLE_OXM
	if (alert_id == ALERT_NONE) {
		if (bpm > 0) {
			if (bpm < 40.0 || bpm > 130.0) {
				return ALERT_HEARTRATE;
			} else if (spo2 > 0 && spo2 < 90) {
				return ALERT_O2;
			}
		}
	}
#endif
	return alert_id;
};
