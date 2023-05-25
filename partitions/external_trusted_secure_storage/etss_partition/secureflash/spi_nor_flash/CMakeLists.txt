#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0079 NEW)

add_library(spi_nor_driver STATIC)
target_include_directories(spi_nor_driver
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}

)

target_sources(spi_nor_driver
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/spi_nor.c
)


target_link_libraries(spi_nor_driver
    PRIVATE
        secureflash_platform
)

