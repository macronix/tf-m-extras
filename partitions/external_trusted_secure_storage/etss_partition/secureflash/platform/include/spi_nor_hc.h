/*
 * Copyright (c) 2022-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _SPI_NOR_HC_H_
#define _SPI_NOR_HC_H_
#include <stdint.h>

/**
 * \brief Wait for several microseconds.
 *
 * \param[in] microsec  Specific microseconds
 *
 */
void wait_for_us(uint32_t microsec);
/**
 * \brief Wait for several milliseconds.
 *
 * \param[in] millisec  Specific milliseconds
 *
 */
void wait_for_ms(uint32_t millisec);
/**
 * \brief Send write data packet to nor flash.
 *
 * \param[in] tx_buf  Pointer to the packet to send
 * \param[in] tx_len  Size of the packet to send
 * \return: 0 on success, -1 otherwise
 */
int32_t spi_write(uint8_t *tx_buf, uint32_t tx_len);
/**
 * \brief Send read data packet to nor flash.
 *
 * \param[in] tx_buf   Pointer to the packet to send
 * \param[in] tx_len   Size of the packet to send
 * \param[out] rx_buf  Pointer to the packet to receive
 * \param[in] rx_len size of the packet to receive
 * \return: 0 on success, -1 otherwise
 */
int32_t spi_read(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len);
/**
 * \brief Spi nor host controller init.
 *
 * \return: 0 on success, -1 otherwise
 */
int32_t spi_nor_hc_init(void);


#endif /* _SPI_NOR_HC_H_ */
