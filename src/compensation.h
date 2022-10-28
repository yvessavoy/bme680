#ifndef _COMPENSATION_H
#define _COMPENSATION_H

#include "bme680.h"

int32_t compensate_temperature(BME680_t *sensor, uint32_t temperature);
int32_t compensate_humidity(BME680_t *sensor, uint32_t humidity);

#endif