/**\
 * Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/******************************************************************************/
/*!                 Header Files                                              */
#include <stdio.h>
#include "bmp2_defs.h"
#include "common.h"
#include "bmp2.h"
#include "bmp280.h"


/******************************************************************************/
/*!         Static Function Declaration                                       */

/*!
 *  @brief This internal API is used to get compensated pressure and temperature data.
 *
 *  @param[in] period   : Contains the delay in microseconds
 *  @param[in] conf     : Structure instance of bmp2_config.
 *  @param[in] dev      : Structure instance of bmp2_dev.
 *
 *  @return Status of execution.
 */

void bme280_initialize() {
    int8_t rslt;
    uint32_t meas_time;

    rslt = bmp2_interface_selection(&dev, BMP2_SPI_INTF);
    bmp2_error_codes_print_result("bmp2_interface_selection", rslt);

    rslt = bmp2_init(&dev);
    bmp2_error_codes_print_result("bmp2_init", rslt);

    /* Always read the current settings before writing, especially when all the configuration is not modified */
    rslt = bmp2_get_config(&conf, &dev);
    bmp2_error_codes_print_result("bmp2_get_config", rslt);

    /* Configuring the over-sampling mode, filter coefficient and output data rate */
    /* Overwrite the desired settings */
    conf.filter = BMP2_FILTER_OFF;

    /* Over-sampling mode is set as high resolution i.e., os_pres = 8x and os_temp = 1x */
//    conf.os_mode = BMP2_OS_MODE_HIGH_RESOLUTION;
    conf.os_mode = BMP2_OS_MODE_ULTRA_HIGH_RESOLUTION;
    conf.odr = BMP2_ODR_4000_MS;

    rslt = bmp2_set_config(&conf, &dev);
    bmp2_error_codes_print_result("bmp2_set_config", rslt);

    /* Set normal power mode */
    rslt = bmp2_set_power_mode(BMP2_POWERMODE_NORMAL, &conf, &dev);
    bmp2_error_codes_print_result("bmp2_set_power_mode", rslt);

    /* Calculate measurement time in microseconds */
    rslt = bmp2_compute_meas_time(&meas_time, &conf, &dev);
    bmp2_error_codes_print_result("bmp2_compute_meas_time", rslt);
};

struct bmp2_data bme280_get_data() {
    int8_t rslt = BMP2_E_NULL_PTR;
    struct bmp2_status status;
    struct bmp2_data comp_data;

    rslt = bmp2_get_status(&status, &dev);
    if (status.measuring == BMP2_MEAS_DONE)
    {    
        rslt = bmp2_get_sensor_data(&comp_data, &dev);
        bmp2_error_codes_print_result("bmp2_get_sensor_data", rslt);
        return comp_data;
    }
}

int8_t bmp280_get_status() {
    struct bmp2_status status;
    return bmp2_get_status(&status, &dev);
}
