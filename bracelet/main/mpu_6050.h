/* This headers uses le STB header hack :3 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buntypes.h"
#include "driver/i2c_master.h"
#include "buntypes.h"

#define MPU6050_I2C_ADDR 0x68
#define MPU6050_REG_START 0x3B
#define MPU6050_REG_PWR_MGMT 0x6B
#define I2C_MASTER_TIMEOUT pdMS_TO_TICKS(100)

typedef struct {
	i16 acc_x, acc_y, acc_z;
	i16 temp;
	i16 gyro_x, gyro_y, gyro_z;

} mpu6050_data;

void mpu6050_wake_up(i2c_master_dev_handle_t handle);

esp_err_t mpu6050_read_all(i2c_master_dev_handle_t dev_handle, mpu6050_data *data);

#ifdef MPU_6050_IMPL

esp_err_t mpu6050_read_all(i2c_master_dev_handle_t dev_handle, mpu6050_data *data)
{
	u8 start_reg = MPU6050_REG_START;
	u8 raw_buffer[14]; // Temporary buffer for the 14 sequential raw bytes

	// Transmit starting register address, then receive 14 sequential data bytes
	esp_err_t err = i2c_master_transmit_receive(dev_handle, &start_reg, sizeof(start_reg),
						    raw_buffer, sizeof(raw_buffer),
						    I2C_MASTER_TIMEOUT);

	if (err == ESP_OK) {
		// Bit shift buffers and update struct
		data->acc_x = (raw_buffer[0] << 8) | raw_buffer[1];
		data->acc_y = (raw_buffer[2] << 8) | raw_buffer[3];
		data->acc_z = (raw_buffer[4] << 8) | raw_buffer[5];
		data->temp = (raw_buffer[6] << 8) | raw_buffer[7];
		data->gyro_x = (raw_buffer[8] << 8) | raw_buffer[9];
		data->gyro_y = (raw_buffer[10] << 8) | raw_buffer[11];
		data->gyro_z = (raw_buffer[12] << 8) | raw_buffer[13];
	}
	return err;
}

void mpu6050_wake_up(i2c_master_dev_handle_t handle){
	u8 wake_cmd[2] = { MPU6050_REG_PWR_MGMT, 0x00 };
	ESP_ERROR_CHECK(i2c_master_transmit(handle, wake_cmd, sizeof(wake_cmd),
					    pdMS_TO_TICKS(100)));

}

#endif


