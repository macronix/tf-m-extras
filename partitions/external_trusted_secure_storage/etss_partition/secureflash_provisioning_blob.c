/*
 * Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#if defined(SECUREFLASH_PROVISION) && defined(ETSS_PROV_DEVELOPER_MODE)
#include <stdint.h>
const uint8_t PROVISIONING_BLOB[] =
{
     /* Major Header */
     'S',  'F',  'P',  'I',  // DWORD 0: [31:00] magic, "SFPI"
     0x01, 0x01, 0x3c, 0x00, // DWORD 1: [03:00] minor_version, [07:04] major_version, [15:08] sub header number, [31:16] total size
     0xc2, 0x29, 0x17, 0x00, // DWORD 2: [23:00] ID, [31:24] provision_disable_indicator

     /* Sub Header, ID: 0x03, APP_INFO */
     0x03, 0x04, 0x30, 0x80, // DWORD 0: [07:00] ID, [15:08] num, [30:16] app_info size, [31] save:1, not save: 0
     /* APP_INFO, Table */
     0xFE, 0xFF, 0xFF, 0xFF, // DWORD  1: [31:00] application id            (-2/0xfffffffe)
     0x55, 0xAA, 0x55, 0x00, // DWORD  2: [31:00] key id                    (app0)
     0x00, 0xFF, 0xFF, 0xFF, // DWORD  3: [07:00] zone id, [31:08] reserved (app0)
     0xFD, 0xFF, 0xFF, 0xFF, // DWORD  1: [31:00] application id            (-3/0xfffffffd)
     0x55, 0xAA, 0x55, 0x04, // DWORD  5: [31:00] key id                    (app1)
     0x01, 0xFF, 0xFF, 0xFF, // DWORD  6: [07:00] zone id, [31:08] reserved (app1)
     0xBA, 0x0B, 0x00, 0x00, // DWORD  1: [31:00] application id            (3002/0x00000bba)
     0x55, 0xAA, 0x55, 0x0B, // DWORD  8: [31:00] key id                    (app2)
     0x02, 0xFF, 0xFF, 0xFF, // DWORD  9: [07:00] zone id, [31:08] reserved (app2)
     0xBE, 0x0B, 0x00, 0x00, // DWORD 10: [31:00] application id            (3006/0x00000bbe)
     0x55, 0xAA, 0x55, 0x0F, // DWORD 11: [31:00] key id                    (app3)
     0x03, 0xFF, 0xFF, 0xFF, // DWORD 12: [07:00] zone id, [31:08] reserved (app3)
};

const uint32_t PROVISIONING_BLOB_SIZE = sizeof(PROVISIONING_BLOB);
#endif
