#include "soc/gpio_num.h"
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gpio_etm.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

bool started = false;

static void setup_configs(void);
static void setup_network(void);
static void setup_tasks(void);

static void i2c_task(void);
static void btn_task(void);
static void net_task(void);

static void clean_and_exit(void);

void app_main(void)
{
	if (!started) {
		setup_configs();
		setup_tasks();
	}
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void setup_configs(void)
{
	// GPIO4 - PULL UP Alert button w/ hardware interrupt

	// I2C
	// - ACCELEROMETER ADDR 0x68
	// - OXIMETER ADDR 0x57

	// GPIO22 (SDA), GPIO21 (SCL)
}

static void setup_tasks(void)
{
	// setup watchdog timers
	// i2c task
	// emergency button task
	// network Task
}

static void btn_task(void)
{
}

static void i2c_task(void)
{
}

static void clean_and_exit(void)
{
}
