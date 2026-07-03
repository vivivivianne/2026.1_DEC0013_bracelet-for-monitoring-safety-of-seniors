#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "driver/gpio.h"
#include "esp_wifi.h"

#define MPU_6050_IMPL
#include "mpu_6050.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

#define WIFI_SSID "wifi"
#define WIFI_PASSWD "senha123"

#define ALERT_PIN GPIO_NUM_4 
#define ALERT_MASK (1ULL << ALERT_PIN)
#define I2C_PORT I2C_NUM_0
#define SDA GPIO_NUM_22
#define SCL GPIO_NUM_21

#define OXM_ADDR 0x57

static bool trigger_alert = false;

typedef struct {
	i2c_master_bus_handle_t *bus;
	i2c_master_dev_handle_t *accel;
	i2c_master_dev_handle_t *oxm;
} i2c_handles;

static void setup_configs(i2c_handles handles);
static void setup_tasks(i2c_handles handles);
static void i2c_task(void *pvParameters);
static void btn_isr_handler(void *arg);
static void btn_task(void);

void app_main(void)
{
	i2c_handles handles = {};
	setup_configs(handles);
	setup_tasks(handles);

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void setup_configs(i2c_handles handles)
{
        //Setup Wifi
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASSWD,
                .bssid_set = false,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

	// GPIO4 - PULL UP Alert button w/ hardware interrupt
	gpio_config_t conf = {
		.pin_bit_mask = ALERT_MASK,
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_NEGEDGE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_ENABLE,
	};
	gpio_config(&conf);

	gpio_install_isr_service(ESP_INTR_FLAG_LEVEL2);
	gpio_isr_handler_add(ALERT_PIN, btn_isr_handler, (void *)ALERT_PIN);

	// I2C Setup for both the accelerometer and oximeter
	i2c_master_bus_config_t i2c_mst_config = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = I2C_PORT,
		.scl_io_num = SCL,
		.sda_io_num = SDA,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
		// just for now, probably add some proper resistors to the circuit later
	};

	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, handles.bus));

	i2c_device_config_t accel_conf = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = MPU6050_I2C_ADDR,
		.scl_speed_hz = 400000,
	};

	i2c_device_config_t oxm_conf = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = OXM_ADDR,
		.scl_speed_hz = 400000,
	};

	ESP_ERROR_CHECK(i2c_master_bus_add_device(*handles.bus, &accel_conf, handles.accel));
	ESP_ERROR_CHECK(i2c_master_bus_add_device(*handles.bus, &oxm_conf, handles.oxm));

	// device is in eepy mode by default
	mpu6050_wake_up(*handles.accel);

	// check if the oximeter needs that too later
}

static void btn_isr_handler(void *arg)
{
	// probably be careful with semaphores around this variable?
	trigger_alert = true;
}

static void setup_tasks(i2c_handles handles)
{
	xTaskCreate(i2c_task, "i2c_task", 4096, (void *)&handles, 5, NULL);
        ESP_ERROR_CHECK(esp_wifi_start());
	// emergency button task
	// network Task
}

static void i2c_task(void *pvParameters)
{
	i2c_handles handles = *(i2c_handles *)pvParameters;

	TickType_t t = xTaskGetTickCount();
	TickType_t dt = pdMS_TO_TICKS(10);

	mpu6050_data data;

	while (true) {
		if (mpu6050_read_all(*handles.accel, &data) == ESP_OK) {
			printf("Accel X: %d | Accel Y: %d | Accel Z: %d\n", data.acc_x, data.acc_y,
			       data.acc_z);
			printf("Gyro X: %d | Gyro Y: %d | Gyro Z: %d\n", data.gyro_x, data.gyro_y,
			       data.gyro_z);
		} else {
			ESP_LOGE("MPU6050", "Failed to read data over I2C");
		}
		vTaskDelayUntil(&t, dt);
	}
}

static void i2c_register_read(i2c_master_dev_handle_t handle, u8 reg_addr, u8 *buffer,
			      u8 buffer_size)
{
	i2c_master_transmit_receive(handle, &reg_addr, 1, buffer, buffer_size, I2C_MASTER_TIMEOUT);
}
