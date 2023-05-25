#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0079 NEW)

if (NOT TEST_S_ETSS)
    return()
endif()

target_sources(tfm_test_suite_extra_s
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/etss_s_interface_testsuite.c
)

target_include_directories(tfm_test_suite_extra_s
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(tfm_test_suite_extra_s
    PRIVATE
        ETSS_MAX_ASSET_SIZE=${ETSS_MAX_ASSET_SIZE}
)