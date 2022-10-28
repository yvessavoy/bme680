#include "compensation.h"

int32_t compensate_temperature(BME680_t *sensor, uint32_t temperature)
{
    int32_t var1 = 0, var2 = 0, var3 = 0, T = 0;
    var1 = ((int32_t)temperature >> 3) - ((int32_t)sensor->compensation.dig_T1 << 1);
    var2 = (var1 * (int32_t)sensor->compensation.dig_T2) >> 11;
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)sensor->compensation.dig_T3 << 4)) >> 14;
    sensor->compensation.t_fine = var2 + var3;
    T = (sensor->compensation.t_fine * 5 + 128) >> 8;
    return T;
}

int32_t compensate_humidity(BME680_t *sensor, uint32_t humidity)
{
    int32_t var1 = 0, var2 = 0, var3 = 0, var4 = 0, var5 = 0, var6 = 0, H = 0, T = 0;

    T = (((int32_t)sensor->compensation.t_fine * 5) + 128) >> 8;
    var1 = (int32_t)humidity - ((int32_t)((int32_t)sensor->compensation.dig_H1 << 4)) - (((T * (int32_t)sensor->compensation.dig_H3) / ((int32_t)100)) >> 1);
    var2 = ((int32_t)sensor->compensation.dig_H2 * (((T * (int32_t)sensor->compensation.dig_H4) /
                                                     ((int32_t)100)) +
                                                    (((T * ((T * (int32_t)sensor->compensation.dig_H5) /
                                                            ((int32_t)100))) >>
                                                      6) /
                                                     ((int32_t)100)) +
                                                    (int32_t)(1 << 14))) >>
           10;
    var3 = var1 * var2;
    var4 = ((((int32_t)sensor->compensation.dig_H6) << 7) + ((T * (int32_t)sensor->compensation.dig_H7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;

    H = (var3 + var6) >> 12;

    if (H > 102400)
        H = 102400; // check for over- and under-flow
    else if (H < 0)
        H = 0;

    return H;
}