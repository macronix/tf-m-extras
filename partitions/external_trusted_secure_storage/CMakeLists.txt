#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TFM_PARTITION_EXTERNAL_TRUSTED_SECURE_STORAGE)
    return()
endif()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)

set(ETSS_PARTITION_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/etss_partition)
include(${ETSS_PARTITION_DIR}/etss_config.cmake)

add_library(tfm_psa_rot_partition_etss STATIC)
add_subdirectory(${ETSS_PARTITION_DIR}/secureflash)
target_include_directories(tfm_psa_rot_partition_etss
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/external_trusted_secure_storage
        ${ETSS_PARTITION_DIR}/secureflash_fs
        ${ETSS_PARTITION_DIR}/external_secure_flash
    INTERFACE
        ${ETSS_PARTITION_DIR}/secureflash/vendor_impl/${SECUREFLASH_TYPE}
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/external_trusted_secure_storage
)

target_include_directories(tfm_config
    INTERFACE
        ${ETSS_PARTITION_DIR}
)

target_sources(tfm_psa_rot_partition_etss
    PRIVATE
        ${ETSS_PARTITION_DIR}/etss_utils.c
        ${ETSS_PARTITION_DIR}/etss_req_mngr.c
        ${ETSS_PARTITION_DIR}/external_trusted_secure_storage.c
        ${ETSS_PARTITION_DIR}/external_secure_flash/etss_secureflash.c
        ${ETSS_PARTITION_DIR}/secureflash_fs/etss_flash_fs.c
        ${ETSS_PARTITION_DIR}/secureflash_fs/etss_flash_fs_dblock.c
        ${ETSS_PARTITION_DIR}/secureflash_fs/etss_flash_fs_mblock.c
)

# The generated sources
target_sources(tfm_psa_rot_partition_etss
    PRIVATE
            ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/external_trusted_secure_storage/auto_generated/intermedia_etss.c
)
target_sources(tfm_partitions
    INTERFACE
            ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/external_trusted_secure_storage/auto_generated/load_info_etss.c
)

target_link_libraries(tfm_psa_rot_partition_etss
    PRIVATE
        tfm_sprt
        platform_s
        psa_interface
        hw_secure_flash
        tfm_config
)

#Add compile definitions to a target
target_compile_definitions(tfm_psa_rot_partition_etss
    PRIVATE
        $<$<BOOL:${ETSS_CREATE_FLASH_LAYOUT}>:ETSS_CREATE_FLASH_LAYOUT>
        $<$<BOOL:${ETSS_VALIDATE_METADATA_FROM_FLASH}>:ETSS_VALIDATE_METADATA_FROM_FLASH>
        ETSS_MAX_ASSET_SIZE=${ETSS_MAX_ASSET_SIZE}
        ETSS_NUM_ASSETS=${ETSS_NUM_ASSETS}
        PROV_BLOB_LEN_MAX=${PROV_BLOB_LEN_MAX}
        $<$<BOOL:${ETSS_BUF_SIZE}>:ETSS_BUF_SIZE=${ETSS_BUF_SIZE}>
        $<$<BOOL:${SECUREFLASH_PROVISION}>:SECUREFLASH_PROVISION>
        $<$<BOOL:${MULTI_CLIENT_ISOLATION}>:MULTI_CLIENT_ISOLATION>
        ETSS_PROV_DEVELOPER_MODE=${ETSS_PROV_DEVELOPER_MODE}
)
################ Display the configuration being applied #######################

message(STATUS "----------- Display storage configuration - start ------------")
message(STATUS "ETSS_CREATE_FLASH_LAYOUT is set to ${ETSS_CREATE_FLASH_LAYOUT}")
message(STATUS "ETSS_VALIDATE_METADATA_FROM_FLASH is set to ${ETSS_VALIDATE_METADATA_FROM_FLASH}")
message(STATUS "ETSS_MAX_ASSET_SIZE is set to ${ETSS_MAX_ASSET_SIZE}")
message(STATUS "ETSS_NUM_ASSETS is set to ${ETSS_NUM_ASSETS}")
if (${ETSS_BUF_SIZE})
    message(STATUS "ETSS_BUF_SIZE is set to ${ETSS_BUF_SIZE}")
else()
    message(STATUS "ETSS_BUF_SIZE is not set (defaults to ETSS_MAX_ASSET_SIZE)")
endif()

message(STATUS "----------- Display storage configuration - stop -------------")

############################ Secure API ########################################

set(INTERFACE_SRC_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/interface/src)
set(INTERFACE_INC_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/interface/include)

target_sources(tfm_sprt
    PRIVATE
        ${INTERFACE_SRC_DIR}/etss_secure_api.c
)

target_include_directories(tfm_sprt
    PUBLIC
        ${INTERFACE_INC_DIR}
)

# The veneers give warnings about not being properly declared so they get hidden
# to not overshadow _real_ warnings.
set_source_files_properties(tfm_etss_secure_api.c
    PROPERTIES
        COMPILE_FLAGS -Wno-implicit-function-declaration
)

############################ Non-secure API ####################################

add_library(etss_api_ns INTERFACE)

target_sources(etss_api_ns
    INTERFACE
        ${INTERFACE_SRC_DIR}/etss_ipc_api.c
)

target_include_directories(etss_api_ns
    INTERFACE
        ${INTERFACE_INC_DIR}
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_etss
)

target_compile_definitions(tfm_partition_defs
    INTERFACE
        TFM_PARTITION_EXTERNAL_TRUSTED_SECURE_STORAGE
)
target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_EXTERNAL_TRUSTED_SECURE_STORAGE
)