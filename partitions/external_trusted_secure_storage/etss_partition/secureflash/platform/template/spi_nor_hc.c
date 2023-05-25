/*
 * Copyright (c) 2022-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "spi_nor_hc.h"
#include "Driver_SPI.h"
#define SPI_NOR_HOST_DRIVER  (Driver_SPI3)
extern ARM_DRIVER_SPI        SPI_NOR_HOST_DRIVER;

void wait_for_us(uint32_t microsec)
{
    //TODO
}

void wait_for_ms(uint32_t millisec)
{
    millisec = HAL_GetTick() + millisec;
    while(HAL_GetTick() < millisec);
}

int32_t spi_write(uint8_t *tx_buf, uint32_t tx_len)
{
    SPI_NOR_HOST_DRIVER.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
    SPI_NOR_HOST_DRIVER.Send(tx_buf, tx_len);
    SPI_NOR_HOST_DRIVER.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
    return 0;
}

int32_t spi_read(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len)
{
    SPI_NOR_HOST_DRIVER.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
    SPI_NOR_HOST_DRIVER.Send(tx_buf, tx_len);
    SPI_NOR_HOST_DRIVER.Receive(rx_buf, (int32_t)rx_len);
    SPI_NOR_HOST_DRIVER.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
    return 0;
}

int32_t spi_nor_hc_init(void)
{
    if (SPI_NOR_HOST_DRIVER.Initialize(NULL)) {
        return -1;
    }
    return 0;
}
