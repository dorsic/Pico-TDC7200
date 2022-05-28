/**\
 * Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "bmp2_defs.h"

/******************************************************************************/
/*!                Static variable definition                                 */

/*! Variable that holds the I2C device address or SPI chip selection */
static uint8_t dev_addr;

#define MISO 16
#define CS   26
#define SCLK 18
#define MOSI 19
#define SPI_PORT spi0

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to COINES platform
 */
BMP2_INTF_RET_TYPE bmp2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    dev_addr = *(uint8_t*)intf_ptr;

    //return coines_read_i2c(dev_addr, reg_addr, reg_data, (uint16_t)length);
    return BMP2_E_DEV_NOT_FOUND;
}

/*!
 * I2C write function map to COINES platform
 */
BMP2_INTF_RET_TYPE bmp2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    dev_addr = *(uint8_t*)intf_ptr;

   // return coines_write_i2c(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)length);
   return BMP2_E_DEV_NOT_FOUND;
}

/*!
 * SPI read function map to COINES platform
 */
BMP2_INTF_RET_TYPE bmp2_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
//    dev_addr = *(uint8_t*)intf_ptr;
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, &reg_addr, 1);
    spi_read_blocking(SPI_PORT, 0, reg_data, (uint16_t)length);
    gpio_put(CS, 1);
    return BMP2_INTF_RET_SUCCESS;
//    return coines_read_spi(dev_addr, reg_addr, reg_data, (uint16_t)length);
}

/*!
 * SPI write function map to COINES platform
 */
BMP2_INTF_RET_TYPE bmp2_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    dev_addr = *(uint8_t*)intf_ptr;
    gpio_put(CS, 0);
    //(reg_addr &= 0x7F)
    spi_write_blocking(SPI_PORT, &reg_addr, 1);
    spi_write_blocking(SPI_PORT, reg_data, length);
    gpio_put(CS, 1);
    return BMP2_INTF_RET_SUCCESS;
    //return coines_write_spi(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)length);
}

/*!
 * Delay function map to COINES platform
 */
void bmp2_delay_us(uint32_t period, void *intf_ptr)
{
    sleep_us(period); 
}

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bmp2_error_codes_print_result(const char api_name[], int8_t rslt)
{
    if (rslt != BMP2_OK)
    {
        printf("%s\t", api_name);

        switch (rslt)
        {
            case BMP2_E_NULL_PTR:
                printf("Error [%d] : Null pointer error.", rslt);
                printf(
                    "It occurs when the user tries to assign value (not address) to a pointer, which has been initialized to NULL.\r\n");
                break;
            case BMP2_E_COM_FAIL:
                printf("Error [%d] : Communication failure error.", rslt);
                printf(
                    "It occurs due to read/write operation failure and also due to power failure during communication\r\n");
                break;
            case BMP2_E_INVALID_LEN:
                printf("Error [%d] : Invalid length error.", rslt);
                printf("Occurs when length of data to be written is zero\n");
                break;
            case BMP2_E_DEV_NOT_FOUND:
                printf("Error [%d] : Device not found error. It occurs when the device chip id is incorrectly read\r\n",
                       rslt);
                break;
            case BMP2_E_UNCOMP_TEMP_RANGE:
                printf("Error [%d] : Uncompensated temperature data not in valid range error.", rslt);
                break;
            case BMP2_E_UNCOMP_PRESS_RANGE:
                printf("Error [%d] : Uncompensated pressure data not in valid range error.", rslt);
                break;
            case BMP2_E_UNCOMP_TEMP_AND_PRESS_RANGE:
                printf(
                    "Error [%d] : Uncompensated pressure data and uncompensated temperature data are not in valid range error.",
                    rslt);
                break;
            default:
                printf("Error [%d] : Unknown error code\r\n", rslt);
                break;
        }
    }
}

/*!
 *  @brief Function to select the interface between SPI and I2C.
 */
int8_t bmp2_interface_selection(struct bmp2_dev *dev, uint8_t intf)
{
    int8_t rslt = BMP2_OK;

    if (dev != NULL)
    {
        sleep_ms(100);

        /* Bus configuration : I2C */
        if (intf == BMP2_I2C_INTF)
        {
            printf("I2C Interface\n");

            dev_addr = BMP2_I2C_ADDR_PRIM;
            dev->read = bmp2_i2c_read;
            dev->write = bmp2_i2c_write;
            dev->intf = BMP2_I2C_INTF;

//            coines_config_i2c_bus(COINES_I2C_BUS_0, COINES_I2C_STANDARD_MODE);
        }
        /* Bus configuration : SPI */
        else if (intf == BMP2_SPI_INTF)
        {
            printf("SPI Interface\n");

            dev_addr = CS;
            dev->read = bmp2_spi_read;
            dev->write = bmp2_spi_write;
            dev->intf = BMP2_SPI_INTF;

            //Initialise GPIO pins for SPI communication
//            coines_config_spi_bus(COINES_SPI_BUS_0, COINES_SPI_SPEED_7_5_MHZ, COINES_SPI_MODE0);
            spi_init(SPI_PORT, 1 * 1000000); // Initialise spi0 at 5000kHz
            gpio_set_function(MISO, GPIO_FUNC_SPI);
            gpio_set_function(SCLK, GPIO_FUNC_SPI);
            gpio_set_function(MOSI, GPIO_FUNC_SPI);
            // Configure Chip Select
            gpio_init(CS); // Initialise CS Pin
            gpio_set_dir(CS, GPIO_OUT); // Set CS as output
            gpio_put(CS, 1); // Set CS High to indicate no currect SPI communication            
        }

        /* Holds the I2C device addr or SPI chip selection */
        dev->intf_ptr = &dev_addr;

        /* Configure delay in microseconds */
        dev->delay_us = bmp2_delay_us;
        sleep_ms(200);
    }
    else
    {
        rslt = BMP2_E_NULL_PTR;
    }

    return rslt;
}

/*!
 *  @brief Function deinitializes coines platform.
 */
void bmp2_coines_deinit(void)
{
    //coines_close_comm_intf(COINES_COMM_INTF_USB);
}
