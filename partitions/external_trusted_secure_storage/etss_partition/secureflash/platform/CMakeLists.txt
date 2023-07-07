#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0079 NEW)

add_library(secureflash_platform STATIC)

target_sources(secureflash_platform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/template/spi_nor_hc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/template/plat_secure_flash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/template/low_level_spi.c
)

target_include_directories(secureflash_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/template
)

target_link_libraries(secureflash_platform
    PRIVATE
        platform_s
)