/*
 * Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "plat_secure_flash.h"
#include "Driver_Flash.h"
#include "low_level_ospi_flash.h"
#include "flash_layout.h"


#define SECUREFLASH_PROV_INFO_OFFSET (0x10000)
static uint8_t initialized = 0;
extern ARM_DRIVER_FLASH OSPI_FLASH_DEV_NAME;

/* Platform specific implementation of getting pre-provisioned
 * secure Flash provisioning information
 */
int32_t plat_get_secure_flash_provision_info(uint8_t *buffer, uint32_t size)
{
    if (!initialized) {
        if (OSPI_FLASH_DEV_NAME.Initialize(NULL) != ARM_DRIVER_OK) {
            return -1;
        }
        initialized = 1;
    }
    OSPI_FLASH_DEV_NAME.ReadData(SECUREFLASH_PROV_INFO_OFFSET, buffer, size);
    return 0;
}

/* Platform specific implementation of storing secure Flash
 * provisioning information
 */
int32_t plat_store_secure_flash_provision_info(uint8_t *buffer, uint32_t size)
{
    if (!initialized) {
        if (OSPI_FLASH_DEV_NAME.Initialize(NULL) != ARM_DRIVER_OK) {
            return -1;
        }
        initialized = 1;
    }
    if (OSPI_FLASH_DEV_NAME.EraseSector(SECUREFLASH_PROV_INFO_OFFSET) != ARM_DRIVER_OK) {
        return -1;
    }
    if (OSPI_FLASH_DEV_NAME.ProgramData(SECUREFLASH_PROV_INFO_OFFSET, buffer, size) != ARM_DRIVER_OK) {
        return -1;
    }
    return 0;
}
