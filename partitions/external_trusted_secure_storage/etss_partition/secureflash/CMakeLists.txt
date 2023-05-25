#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0079 NEW)

add_library(hw_secure_flash STATIC)
target_include_directories(hw_secure_flash
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/JEDEC_security_HAL
        ${CMAKE_CURRENT_SOURCE_DIR}/spi_nor_flash
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${TFM_PATH}/platform/ext/driver
        ${CMAKE_CURRENT_SOURCE_DIR}/JEDEC_security_HAL/vendor_impl/${SECUREFLASH_TYPE}
)
add_subdirectory(platform)
add_subdirectory(spi_nor_flash)
add_subdirectory(JEDEC_security_HAL)

target_sources(hw_secure_flash
    PRIVATE
        secureflash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spi_nor_flash/spi_nor.c
)

if(SECUREFLASH_PROVISION)
target_include_directories(hw_secure_flash
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor_impl
)
endif()


#Add compile definitions to a target
target_compile_definitions(hw_secure_flash
    PRIVATE
        ETSS_SF_WRITE_BUF_SIZE=${ETSS_SF_ASSET_BUF_SIZE}
        ETSS_SF_READ_BUF_SIZE=${ETSS_SF_ASSET_BUF_SIZE}
        CRYPTO_ASSET_SIZE=${ETSS_SF_ASSET_BUF_SIZE}
        $<$<BOOL:${SECUREFLASH_PROVISION}>:SECUREFLASH_PROVISION>
        ETSS_PROV_DEVELOPER_MODE=${ETSS_PROV_DEVELOPER_MODE}
)

target_link_libraries(hw_secure_flash
    PRIVATE
        psa_interface
        platform_s
        tfm_sprt
        jedec_security_hal
        secureflash_platform
    INTERFACE
        platform_common_interface
)
