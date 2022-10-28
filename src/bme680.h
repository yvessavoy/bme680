#ifndef _BME680_H
#define _BME680_H

#include "esp_err.h"

#define BME680_I2C_ADDR 0x77

#define BME680_REG_ID 0xD0
#define BME680_REG_RESET 0xE0
#define BME680_REG_CTRL_MEAS 0x74
#define BME680_REG_TEMP_MSB 0x22
#define BME680_REG_TEMP_LSB 0x23
#define BME680_REG_TEMP_XLSB 0x24
#define BME680_REG_HUM_MSB 0x25
#define BME680_REG_HUM_LSB 0x26

typedef struct
{
    uint8_t dig_P10, dig_H6;
    uint16_t dig_T1, dig_P1, dig_H1, dig_H2;
    int16_t dig_T2, dig_P2, dig_P4, dig_P5, dig_P8, dig_P9, dig_GH2;
    int8_t dig_T3, dig_P3, dig_P6, dig_P7, dig_H3, dig_H4, dig_H5, dig_H7, dig_GH1, dig_GH3;
    int32_t t_fine;
} Compensation_t;

typedef struct
{
    int i2c_port;
    uint8_t chip_id;
    float temperature;
    float humidity;
    float gas_resistance;
    Compensation_t compensation;
} BME680_t;

// Returns ESP_OK on sucess, ESP_FAIL on failed initialization
esp_err_t BME680_Init(BME680_t *sensor, int scl, int sda);

// Trigger a measurement and populate the values in the
// given `BME680_t`-parameter
// Returns ESP_OK on sucess, ESP_FAIL on failed measurement
// The values in BME680_t are only valid if this
// function returns ESP_OK
esp_err_t BME680_Measure(BME680_t *sensor);

esp_err_t BME680_GetChipId(BME680_t *sensor);

#endif