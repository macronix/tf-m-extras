#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023 Macronix. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(TFM_PARTITION_EXTERNAL_TRUSTED_SECURE_STORAGE     ON          CACHE BOOL      "Enable ETSS partition")
set(TFM_PATH                                          "/home/a/workspace1/TF-M/trustedfirmware-m/TF-M-V1.7/trusted-firmware-m"  CACHE PATH    "Absolute path of TF-M project")
set(SECURE_FLASH_TYPE                                 "macronix/armorflash_mx78"   CACHE STRING    "The specific secure Flash type")
set(SECUREFLASH_DEBUG                                 ON          CACHE BOOL      "Enable secure Flash debug")
set(SECUREFLASH_PROVISION                             OFF         CACHE BOOL      "Enable secure Flash provisioning")
set(ETSS_PROV_DEVELOPER_MODE                          OFF         CACHE BOOL      "Enable secure Flash provisioning developer mode")
set(ETSS_CREATE_FLASH_LAYOUT                          ON          CACHE BOOL      "Create flash FS if it doesn't exist for External Trusted Secure Storage partition")
set(ETSS_VALIDATE_METADATA_FROM_FLASH                 ON          CACHE BOOL      "Validate filesystem metadata every time it is read from external secure flash")
set(ETSS_MAX_ASSET_SIZE                               "2048"      CACHE STRING    "The maximum asset size to be stored in the External Trusted Secure Storage area")
set(ETSS_NUM_ASSETS                                   "10"        CACHE STRING    "The maximum number of assets to be stored in the External Trusted Secure Storage area")
set(ETSS_BUF_SIZE                                     ""          CACHE STRING    "Size of the ETSS internal data transfer buffer (defaults to ETSS_MAX_ASSET_SIZE if not set)")
set(PROV_BLOB_LEN_MAX                                 "2048"      CACHE STRING    "The maximum secure flash provision data size")
set(ETSS_SF_ASSET_BUF_SIZE                            "512"       CACHE STRING    "Size of secure Flash read/write buffer")
set(MULTI_CLIENT_ISOLATION                            ON          CACHE BOOL      "Enable multi-client isolation")
