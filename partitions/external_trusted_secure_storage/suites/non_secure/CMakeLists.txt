#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0079 NEW)

if (NOT TEST_NS_ETSS)
    return()
endif()

####################### Non Secure #############################################

target_sources(tfm_test_suite_extra_ns
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ns_test_helpers.c
        ${CMAKE_CURRENT_SOURCE_DIR}/etss_ns_interface_testsuite.c
)

target_include_directories(tfm_test_suite_extra_ns
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(tfm_test_suite_extra_ns
    PRIVATE
        DOMAIN_NS=1
        ETSS_MAX_ASSET_SIZE=${ETSS_MAX_ASSET_SIZE}
        $<$<BOOL:${SECUREFLASH_PROVISION}>:SECUREFLASH_PROVISION>
)

target_link_libraries(tfm_test_suite_extra_ns
    PRIVATE
        etss_api_ns
)