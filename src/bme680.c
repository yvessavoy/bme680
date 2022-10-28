#include "bme680.h"
#include "compensation.h"

#include "esp_log.h"
#include "driver/i2c.h"

esp_err_t BME680_ReadRegister(uint8_t reg_addr, uint8_t *data, size_t data_len)
{
    return i2c_master_write_read_device(0, BME680_I2C_ADDR, &reg_addr, 1, data, data_len, 1000 / portTICK_PERIOD_MS);
}

esp_err_t BME680_WriteRegister(uint8_t reg_addr, uint8_t data)
{
    uint8_t tx_data[2] = {reg_addr, data};

    return i2c_master_write_to_device(0, BME680_I2C_ADDR, tx_data, sizeof(tx_data), 1000 / portTICK_PERIOD_MS);
}

esp_err_t BME680_GetChipId(BME680_t *sensor)
{
    return BME680_ReadRegister(0xD0, &sensor->chip_id, 1);
}

esp_err_t BME680_ReadCompensationValues(BME680_t *sensor)
{
    uint8_t comp[41];
    esp_err_t ret = BME680_ReadRegister(0x89, &comp[0], 25);
    if (ret != ESP_OK)
        return ret;

    ret = BME680_ReadRegister(0xE1, &comp[25], 16);
    if (ret != ESP_OK)
        return ret;

    sensor->compensation.dig_T1 = (uint16_t)(((uint16_t)comp[34] << 8) | comp[33]);
    sensor->compensation.dig_T2 = (int16_t)(((int16_t)comp[2] << 8) | comp[1]);
    sensor->compensation.dig_T3 = (int8_t)(comp[3]);
    // pressure compensation parameters
    sensor->compensation.dig_P1 = (uint16_t)(((uint16_t)comp[6] << 8) | comp[5]);
    sensor->compensation.dig_P2 = (int16_t)(((int16_t)comp[8] << 8) | comp[7]);
    sensor->compensation.dig_P3 = (int8_t)(comp[9]);
    sensor->compensation.dig_P4 = (int16_t)(((int16_t)comp[12] << 8) | comp[11]);
    sensor->compensation.dig_P5 = (int16_t)(((int16_t)comp[14] << 8) | comp[13]);
    sensor->compensation.dig_P6 = (int8_t)(comp[16]);
    sensor->compensation.dig_P7 = (int8_t)(comp[15]);
    sensor->compensation.dig_P8 = (int16_t)(((int16_t)comp[20] << 8) | comp[19]);
    sensor->compensation.dig_P9 = (int16_t)(((int16_t)comp[22] << 8) | comp[21]);
    sensor->compensation.dig_P10 = (uint8_t)(comp[23]);
    // humidity compensation parameters
    sensor->compensation.dig_H1 = (uint16_t)(((uint16_t)comp[27] << 4) | (comp[26] & 0x0F));
    sensor->compensation.dig_H2 = (uint16_t)(((uint16_t)comp[25] << 4) | (comp[26] >> 4));
    sensor->compensation.dig_H3 = (int8_t)comp[28];
    sensor->compensation.dig_H4 = (int8_t)comp[29];
    sensor->compensation.dig_H5 = (int8_t)comp[30];
    sensor->compensation.dig_H6 = (uint8_t)comp[31];
    sensor->compensation.dig_H7 = (int8_t)comp[32];
    // gas sensor compensation parameters
    sensor->compensation.dig_GH1 = (int8_t)comp[37];
    sensor->compensation.dig_GH2 = (int16_t)(((int16_t)comp[36] << 8) | comp[35]);
    sensor->compensation.dig_GH3 = (int8_t)comp[38];

    ESP_LOGI("BME680", "DIG_T1: %d", sensor->compensation.dig_T1);
    ESP_LOGI("BME680", "DIG_T2: %d", sensor->compensation.dig_T2);
    ESP_LOGI("BME680", "DIG_T3: %d", sensor->compensation.dig_T3);

    return ESP_OK;
}

esp_err_t BME680_Init(BME680_t *sensor, int scl, int sda)
{
    sensor->i2c_port = 0;
    int i2c_master_port = sensor->i2c_port;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    esp_err_t ret = i2c_param_config(i2c_master_port, &conf);
    if (ret != ESP_OK)
        return ret;

    ret = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
        return ret;

    // Check ChipID to verify that initial communucation is OK
    ret = BME680_GetChipId(sensor);
    if (ret != ESP_OK)
        return ret;

    if (sensor->chip_id != 0x61)
        return ESP_FAIL;

    // Soft reset
    ret = BME680_WriteRegister(BME680_REG_RESET, 0xB6);
    if (ret != ESP_OK)
        return ret;

    ets_delay_us(200000);

    // Set humidity oversampling rate
    BME680_WriteRegister(0x72, 0x07 & 0x01);

    // Read compration values
    return BME680_ReadCompensationValues(sensor);
}

esp_err_t BME680_Measure(BME680_t *sensor)
{
    // Put sensor in "forced"-mode to trigger measurement
    BME680_WriteRegister(BME680_REG_CTRL_MEAS, 2 << 5 | 5 << 2 | 1);
    ets_delay_us(200000);

    uint8_t raw_data[3];

    // Temperature measurement
    BME680_ReadRegister(BME680_REG_TEMP_MSB, &raw_data[0], 3);
    uint32_t raw_temperature = (((uint32_t)raw_data[0] << 16 | (uint32_t)raw_data[1] << 8 | raw_data[2]) >> 4);
    sensor->temperature = compensate_temperature(sensor, raw_temperature) / 100.0;

    // Humidity measurement
    BME680_ReadRegister(BME680_REG_HUM_MSB, &raw_data[0], 2);
    uint16_t raw_humidity = (((uint16_t)raw_data[0] << 8 | raw_data[1]));
    sensor->humidity = compensate_humidity(sensor, raw_humidity) / 1024.0;

    return ESP_OK;
}