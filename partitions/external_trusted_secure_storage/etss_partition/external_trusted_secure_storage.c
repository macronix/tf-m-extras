/*
 * Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdlib.h>
#include <string.h>
#include "external_trusted_secure_storage.h"
#include "etss_req_mngr.h"
#include "psa_manifest/pid.h"
#include "etss_utils.h"
#include "external_secure_flash/etss_secureflash.h"
#include "secureflash_fs/etss_flash_fs.h"
#include "secureflash_error.h"
#include "tfm_sp_log.h"//for debug

#ifndef ETSS_BUF_SIZE
/* By default, set the ETSS buffer size to the max asset size so that all
 * requests can be handled in one iteration.
 * ETSS_MAX_ASSET_SIZE is defined in etss_config.cmake
 */
#define ETSS_BUF_SIZE ETSS_MAX_ASSET_SIZE
#endif

#ifndef ETSS_FILE_ID_SIZE
#define ETSS_FILE_ID_SIZE 12
#endif
#ifndef ETSS_DEFAULT_EMPTY_BUFF_VAL
#define ETSS_DEFAULT_EMPTY_BUFF_VAL 0
#endif

/* Buffer to store asset data from the caller.
 * Note: size must be aligned to the max flash program unit to meet the
 * alignment requirement of the filesystem.
 */
static uint8_t asset_data[ETSS_UTILS_ALIGN(ETSS_BUF_SIZE,
                                           ETSS_FLASH_ALIGNMENT)];
static uint8_t g_fid[ETSS_FILE_ID_SIZE];

/* Secure Flash with several secure regions configured with independent security
 * policy and access permission, caters to ETSS multi-client isolation.
 * The binding of applications and secure regions should also be allocated
 * in corresponding secureflash_layout.h.
 */
#if defined(MULTI_CLIENT_ISOLATION)
    typedef struct etss_fs_ctx_info {
        int32_t client_id; /*!< Client ID */
        etss_flash_fs_ctx_t fs_ctx; /*!< The file system context corresponding to each Client ID */
    }etss_fs_ctx_info_t;
    static struct etss_flash_fs_config_t fs_cfg_etss[SECURE_FLASH_CLIENT_NUM] = {0}; /*!< The configuration of each client's file system */
    static etss_fs_ctx_info_t etss_fs_ctx_tbl[SECURE_FLASH_CLIENT_NUM] = {0}; /*!< The table of clients' file system contexts */
    static etss_flash_fs_ctx_t *fs_ctx_etss_ptr; /*!< The pointer of file system context */
#else
/* If multi-client isolation is disabled, only one file system context is needed.
 */
    static etss_flash_fs_ctx_t fs_ctx_etss;
    static etss_flash_fs_ctx_t *fs_ctx_etss_ptr = &fs_ctx_etss;
#endif
static struct etss_file_info_t g_file_info;
/* Declare secure flash instance */
static secureflash_t secureflash = {
    ._init_ref_count = 0,
    ._is_initialized = false,
};
/* Base configure of ETSS file system */
static struct etss_flash_fs_config_t etss_flash_fs_config_base = {
    .flash_dev = &secureflash,
    .program_unit = ETSS_FLASH_ALIGNMENT,
    .max_file_size = ETSS_UTILS_ALIGN(ETSS_MAX_ASSET_SIZE, ETSS_FLASH_ALIGNMENT),
    .max_num_files = ETSS_NUM_ASSETS + 1, /* Extra file for atomic replacement */
};
/**
 * \brief Maps a pair of client id and uid to a file id.
 *
 * \param[in]  client_id  Identifier of the asset's owner (client)
 * \param[in]  uid        Identifier for the data
 * \param[out] fid        Identifier of the file
 */
static void etss_get_fid(int32_t client_id,
                         psa_storage_uid_t uid,
                         uint8_t *fid)
{
    memcpy(fid, (const void *)&client_id, sizeof(client_id));
    memcpy(fid + sizeof(client_id), (const void *)&uid, sizeof(uid));
}

#if defined(MULTI_CLIENT_ISOLATION)
/**
 * \brief Get the fs_ctx for given client_id.
 *
 * \return Returns ETSS_SUCCESS if there is already a fs_ctx for given fs_ctx,
 *         and ETSS_ERROR_DOES_NOT_EXIST otherwise.
 */
static etss_err_t etss_get_fs_ctx(int32_t client_id, etss_flash_fs_ctx_t **fs_ctx)
{
    for (uint8_t i = 0; i < SECURE_FLASH_CLIENT_NUM; i++) {
        if (etss_fs_ctx_tbl[i].client_id == client_id) {
            *fs_ctx = &(etss_fs_ctx_tbl[i].fs_ctx);
            return ETSS_SUCCESS;
        }
    }
    return ETSS_ERR_DOES_NOT_EXIST;
}
#endif

/**
 * \brief Initialize file system configurations.
 *
 * \return Returns PSA_ERROR_PROGRAMMER_ERROR if there is any configuration error,
 *         and PSA_SUCCESS otherwise.
 */
static etss_err_t init_fs_cfg(void)
{
    /* Retrieve secure flash file system parameters defined in secureflash_layout.h */
    etss_flash_fs_config_base.sector_size = SECURE_FLASH_SECTOR_SIZE;
    etss_flash_fs_config_base.erase_val = SECURE_FLASH_ERASED_VALUE;
    etss_flash_fs_config_base.flash_area_addr = SECURE_FLASH_START_ADDR;
    etss_flash_fs_config_base.block_size = etss_flash_fs_config_base.sector_size * SECURE_FLASH_SECTORS_PER_BLOCK;
    etss_flash_fs_config_base.num_blocks = SECURE_FLASH_DEFAULT_CLIENT_AREA_SIZE / etss_flash_fs_config_base.block_size;
#if defined(MULTI_CLIENT_ISOLATION)
    for (uint8_t i = 0; i < SECURE_FLASH_CLIENT_NUM; i++) {
        /* common cfg*/
        fs_cfg_etss[i].flash_dev = etss_flash_fs_config_base.flash_dev;
        fs_cfg_etss[i].program_unit = etss_flash_fs_config_base.program_unit;
        fs_cfg_etss[i].max_file_size = etss_flash_fs_config_base.max_file_size;
        fs_cfg_etss[i].max_num_files = etss_flash_fs_config_base.max_num_files;
        fs_cfg_etss[i].sector_size = SECURE_FLASH_SECTOR_SIZE;
        fs_cfg_etss[i].erase_val = SECURE_FLASH_ERASED_VALUE;
    }
    /* client 0 specific file system context */
    fs_cfg_etss[0].flash_area_addr = SECURE_FLASH_CLIENT0_AREA_START_ADDR;
    fs_cfg_etss[0].block_size = fs_cfg_etss[0].sector_size * SECURE_FLASH_CLIENT0_SECTORS_PER_BLOCK;
    fs_cfg_etss[0].num_blocks = SECURE_FLASH_CLIENT0_AREA_SIZE / fs_cfg_etss[0].block_size;
    etss_fs_ctx_tbl[0].client_id = SECURE_FLASH_CLIENT0_ID;
#if (SECURE_FLASH_CLIENT_NUM >= 2)
    /* client 1 specific file system context */
    fs_cfg_etss[1].flash_area_addr = SECURE_FLASH_CLIENT1_AREA_START_ADDR;
    fs_cfg_etss[1].block_size = fs_cfg_etss[1].sector_size * SECURE_FLASH_CLIENT1_SECTORS_PER_BLOCK;
    fs_cfg_etss[1].num_blocks = SECURE_FLASH_CLIENT1_AREA_SIZE / fs_cfg_etss[1].block_size;
    etss_fs_ctx_tbl[1].client_id = SECURE_FLASH_CLIENT1_ID;
#endif
#if (SECURE_FLASH_CLIENT_NUM >= 3)
    /* client 2 specific file system context */
    fs_cfg_etss[2].flash_area_addr = SECURE_FLASH_CLIENT2_AREA_START_ADDR;
    fs_cfg_etss[2].block_size = fs_cfg_etss[2].sector_size * SECURE_FLASH_CLIENT2_SECTORS_PER_BLOCK;
    fs_cfg_etss[2].num_blocks = SECURE_FLASH_CLIENT2_AREA_SIZE / fs_cfg_etss[2].block_size;
    etss_fs_ctx_tbl[2].client_id = SECURE_FLASH_CLIENT2_ID;
#endif
#if (SECURE_FLASH_CLIENT_NUM >= 4)
    /* client 3 specific file system context */
    fs_cfg_etss[3].flash_area_addr = SECURE_FLASH_CLIENT3_AREA_START_ADDR;
    fs_cfg_etss[3].block_size = fs_cfg_etss[3].sector_size * SECURE_FLASH_CLIENT3_SECTORS_PER_BLOCK;
    fs_cfg_etss[3].num_blocks = SECURE_FLASH_CLIENT3_AREA_SIZE / fs_cfg_etss[3].block_size;
    etss_fs_ctx_tbl[3].client_id = SECURE_FLASH_CLIENT3_ID;
#endif
#if (SECURE_FLASH_CLIENT_NUM >= 5)
    //TODO
#endif
#endif
    return ETSS_SUCCESS;
}

static etss_err_t etss_flash_fs_init_ctx_and_prepare(etss_flash_fs_ctx_t *fs_ctx,
                                                     const struct etss_flash_fs_config_t *fs_cfg,
                                                     const struct etss_flash_fs_ops_t *fs_ops)
{
    psa_status_t status;
    status = etss_flash_fs_init_ctx(fs_ctx, fs_cfg, fs_ops);
    if (status == PSA_SUCCESS) {
        /* Prepare the ETSS file system */
        status = etss_flash_fs_prepare(fs_ctx);
#ifdef ETSS_CREATE_FLASH_LAYOUT
        /* If ETSS_CREATE_FLASH_LAYOUT is set, it indicates that it is required
         * to create a ETSS flash layout. ETSS service will generate an empty
         * and valid ETSS flash layout to store assets. It will erase all data
         * located in the assigned ETSS memory area before generating the ETSS
         * layout.
         * This flag can be set if the ETSS memory area is located in persistent
         * memory without a previous valid ETSS flash layout in it. That is the
         * case when it is the first time in the device life that the ETSS
         * service is executed.
         */
        if (status != PSA_SUCCESS) {
            /* Remove all data in the ETSS memory area and create a valid ETSS
             * flash layout in that area.
             */
            status = etss_flash_fs_wipe_all(fs_ctx);
            if (status != PSA_SUCCESS) {
                return status;
            }
            /* Attempt to prepare again */
            status = etss_flash_fs_prepare(fs_ctx);
        }
#endif /* ETSS_CREATE_FLASH_LAYOUT */
        if (status == PSA_SUCCESS) {
            return ETSS_SUCCESS;
        }
    }
    return ETSS_ERR_STORAGE_FAILURE;
}

etss_err_t etss_init(void)
{
    int32_t err;
    err = secureflash_init(&secureflash);
    if (err != SECUREFLASH_SUCCESS) {
        if ((err == SECUREFLASH_ERROR_GET_PROVISION_INFO) ||
            (err == SECUREFLASH_ERROR_SECURE_INIT)){
            return ETSS_ERR_SF_UNPROVISIONED;
        } else {
            return ETSS_ERR_SF_INIT;
        }
    }
    if (secureflash.flash_info.flash_profile->security_feature.security_storage) {
        err = init_fs_cfg();
        if (err != ETSS_SUCCESS) {
            return err;
        }
        /* Initialize ETSS file system contexts */
    #if defined(MULTI_CLIENT_ISOLATION)
        for (uint8_t i = 0; i < SECURE_FLASH_CLIENT_NUM; i++) {
            err = etss_flash_fs_init_ctx_and_prepare(&(etss_fs_ctx_tbl[i].fs_ctx),
                                                     &fs_cfg_etss[i],
                                                     &etss_secure_flash_fs_ops);
            if (err != ETSS_SUCCESS) {
                return ETSS_ERR_STORAGE_FAILURE;
            }
        }
    #else
        err = etss_flash_fs_init_ctx_and_prepare(&fs_ctx_etss,
                                                 &etss_flash_fs_config_base,
                                                 &etss_secure_flash_fs_ops);
        if (err != ETSS_SUCCESS) {
            return ETSS_ERR_STORAGE_FAILURE;
        }
    #endif /* defined(MULTI_CLIENT_ISOLATION) */
    }
    return err;
}

#ifdef SECUREFLASH_PROVISION
etss_err_t etss_secure_flash_provisioning(int32_t client_id,
                                          const uint8_t *prov_data,
                                          size_t data_length)
{
	LOG_INFFMT("etss_secure_flash_provisioning\r\n");
    (void)client_id;
    int32_t status;
    status = secureflash_provision(&secureflash, prov_data, data_length);
    if (status) {
        return ETSS_ERR_SF_PROVISION;
    } else {
        return ETSS_SUCCESS;
    }
    /* FIXME:Has been provisioned already */
}
#endif

etss_err_t etss_set(int32_t client_id,
                    psa_storage_uid_t uid,
                    size_t data_length,
                    psa_storage_create_flags_t create_flags)
{
    psa_status_t status;
    size_t write_size, offset;
    uint32_t flags;
    if (secureflash.flash_info.flash_profile->security_feature.security_storage) {
        /* Check that the UID is valid */
        if (uid == ETSS_INVALID_UID) {
            return ETSS_ERR_INVALID_ARGUMENT;
        }
        /* Check that the create_flags does not contain any unsupported flags */
        if (create_flags & ~(PSA_STORAGE_FLAG_WRITE_ONCE |
                           PSA_STORAGE_FLAG_NO_CONFIDENTIALITY |
                           PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION)) {
            return ETSS_ERR_NOT_SUPPORTED;
        }
        /* Set file id */
        memset(g_fid, 0x00, ETSS_FILE_ID_SIZE);
        etss_get_fid(client_id, uid, g_fid);
        /* Pass client id to secure flash operation layer */
        etss_client_id_pass_on(client_id);
#if defined(MULTI_CLIENT_ISOLATION)
        if (etss_get_fs_ctx(client_id, &fs_ctx_etss_ptr)) {
            return ETSS_ERR_NOT_PERMITTED;
        }
#endif
        status = etss_flash_fs_file_get_info(fs_ctx_etss_ptr, g_fid,
                                             &g_file_info);
        if (status == PSA_SUCCESS) {
            /* If the object exists and has the write once flag set, then it
             * cannot be modified.
             */
            if (g_file_info.flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
                return ETSS_ERR_NOT_PERMITTED;
            }
        } else if (status != PSA_ERROR_DOES_NOT_EXIST) {
            /* If the file does not exist, then do nothing.
             * If other error occurred, return it
             */
            return (etss_err_t)status;
        }
        offset = 0;
        flags = (uint32_t)create_flags |
                ETSS_FLASH_FS_FLAG_CREATE | ETSS_FLASH_FS_FLAG_TRUNCATE;
        /* Iteratively read data from the caller and write it to the filesystem,
         * in chunks no larger than the size of the asset_data buffer.
         */
        do {
            /* Write as much of the data as will fit in the asset_data buffer */
            write_size = ETSS_UTILS_MIN(data_length, sizeof(asset_data));
            /* Read asset data from the caller */
            (void)etss_req_mngr_read(asset_data, write_size);
            /* Write to the file in the file system */
            status = etss_flash_fs_file_write(fs_ctx_etss_ptr, g_fid, flags,
                                              data_length, write_size, offset,
                                              asset_data);
            if (status != PSA_SUCCESS) {
                return (etss_err_t)status;
            }
            /* Do not create or truncate after the first iteration */
            flags &= ~(ETSS_FLASH_FS_FLAG_CREATE | ETSS_FLASH_FS_FLAG_TRUNCATE);
            offset += write_size;
            data_length -= write_size;
        } while (data_length > 0);
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_get(int32_t client_id,
                    psa_storage_uid_t uid,
                    size_t data_offset,
                    size_t data_size,
                    size_t *p_data_length)
{
    psa_status_t status;
    size_t read_size;
    if (secureflash.flash_info.flash_profile->security_feature.security_storage) {
        /* Check that the UID is valid */
        if (uid == ETSS_INVALID_UID) {
            return ETSS_ERR_INVALID_ARGUMENT;
        }
        /* Set file id */
        memset(g_fid, 0x00, ETSS_FILE_ID_SIZE);
        etss_get_fid(client_id, uid, g_fid);
        /* Pass client id to underlying etss_secureflash */
        etss_client_id_pass_on(client_id);

    #if defined(MULTI_CLIENT_ISOLATION)
        if (etss_get_fs_ctx(client_id, &fs_ctx_etss_ptr)) {
            return ETSS_ERR_NOT_PERMITTED;
        }
    #endif
        /* Read file info */
        status = etss_flash_fs_file_get_info(fs_ctx_etss_ptr, g_fid,
                                             &g_file_info);
        if (status != PSA_SUCCESS) {
            return (etss_err_t)status;
        }
        /* Boundary check the incoming request */
        if (data_offset > g_file_info.size_current) {
            return ETSS_ERR_INVALID_ARGUMENT;
        }
        /* Copy the object data only from within the file boundary */
        data_size = ETSS_UTILS_MIN(data_size,
                                   g_file_info.size_current - data_offset);
        /* Update the size of the output data */
        *p_data_length = data_size;
        /* Iteratively read data from the filesystem and write it to the caller,
         * in chunks no larger than the size of the asset_data buffer.
         */
        do {
            /* Read as much of the data as will fit in the asset_data buffer */
            read_size = ETSS_UTILS_MIN(data_size, sizeof(asset_data));
            /* Read file data from the filesystem */
            status = etss_flash_fs_file_read(fs_ctx_etss_ptr, g_fid, read_size,
                                             data_offset, asset_data);
            if (status != PSA_SUCCESS) {
                *p_data_length = 0;
                return (etss_err_t)status;
            }
            /* Write asset data to the caller */
            etss_req_mngr_write(asset_data, read_size);
            data_offset += read_size;
            data_size -= read_size;
        } while (data_size > 0);
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_get_info(int32_t client_id, psa_storage_uid_t uid,
                         struct psa_storage_info_t *p_info)
{
    psa_status_t status;
    if (secureflash.flash_info.flash_profile->security_feature.security_storage) {
        /* Check that the UID is valid */
        if (uid == ETSS_INVALID_UID) {
            return ETSS_ERR_INVALID_ARGUMENT;
        }
        /* Set file id */
        memset(g_fid, 0x00, ETSS_FILE_ID_SIZE);
        etss_get_fid(client_id, uid, g_fid);
        etss_client_id_pass_on(client_id);
#if defined(MULTI_CLIENT_ISOLATION)
        if (etss_get_fs_ctx(client_id, &fs_ctx_etss_ptr)) {
            return ETSS_ERR_NOT_PERMITTED;
        }
#endif
        /* Read file info */
        status = etss_flash_fs_file_get_info(fs_ctx_etss_ptr,
                                             g_fid,
                                             &g_file_info);
        if (status != PSA_SUCCESS) {
            return (etss_err_t)status;
        }
        /* Copy file info to the PSA info struct */
        p_info->capacity = g_file_info.size_current;
        p_info->size = g_file_info.size_current;
        p_info->flags = g_file_info.flags;
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_remove(int32_t client_id, psa_storage_uid_t uid)
{
    psa_status_t status;
    if (secureflash.flash_info.flash_profile->security_feature.security_storage) {
        /* Check that the UID is valid */
        if (uid == ETSS_INVALID_UID) {
            return ETSS_ERR_INVALID_ARGUMENT;
        }
        /* Set file id */
        memset(g_fid, 0x00, ETSS_FILE_ID_SIZE);
        etss_get_fid(client_id, uid, g_fid);
        etss_client_id_pass_on(client_id);

#if defined(MULTI_CLIENT_ISOLATION)
        if (etss_get_fs_ctx(client_id, &fs_ctx_etss_ptr)) {
            return ETSS_ERR_NOT_PERMITTED;
        }
#endif
        /* Read file info */
        status = etss_flash_fs_file_get_info(fs_ctx_etss_ptr,
                                             g_fid,
                                             &g_file_info);
        if (status != PSA_SUCCESS) {
            return (etss_err_t)status;
        }
        /* If the object exists and has the write once flag set, then it
         * cannot be deleted.
         */
        if (g_file_info.flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
            return ETSS_ERR_NOT_PERMITTED;
        }
        /* Delete old file from the persistent area */
        status = etss_flash_fs_file_delete(fs_ctx_etss_ptr, g_fid);
        return (etss_err_t)status;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_get_puf(int32_t client_id, size_t buf_size, size_t *puf_len)
{
    uint8_t puf[SECURE_FLASH_MAX_PUF_SIZE];
    int32_t status;
    uint8_t actual_size;
    if (secureflash.flash_info.flash_profile->security_feature.PUF) {
        status = secureflash_get_puf(&secureflash, puf, sizeof(puf), &actual_size, NULL, 0);
        if (status != SECUREFLASH_SUCCESS) {
            return ETSS_ERR_STORAGE_FAILURE;
        }
        buf_size = ETSS_UTILS_MIN(buf_size, actual_size);
        etss_req_mngr_write(puf, buf_size);
        *puf_len = buf_size;
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_generate_random_number(int32_t client_id, size_t buf_size,
                                       size_t *random_len)
{
    uint8_t random[SECURE_FLASH_MAX_TRNG_SIZE];
    int32_t status;
    uint8_t actual_size;
    *random_len = 0;
    if (secureflash.flash_info.flash_profile->security_feature.RNG) {
        while (buf_size > 0) {
            status = secureflash_get_trng(&secureflash, random, buf_size, &actual_size);
            if (status != SECUREFLASH_SUCCESS) {
                return ETSS_ERR_STORAGE_FAILURE;
            }
            actual_size = ETSS_UTILS_MIN(buf_size, actual_size);
            etss_req_mngr_write(random, actual_size);
            buf_size -= actual_size;
            *random_len += actual_size;
        }
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

/*
*Increase monotonic counter by one:
*/
etss_err_t etss_mc_increment(int32_t client_id, uint8_t mc_id)
{
    if (secureflash.flash_info.flash_profile->security_feature.RPMC) {
        if (secureflash_increase_mc(&secureflash, mc_id, client_id) != 0) {
            return ETSS_ERR_STORAGE_FAILURE;
        }
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}

etss_err_t etss_mc_get(int32_t client_id, uint8_t mc_id, size_t size)
{
    uint8_t mc[SECURE_FLASH_MAX_MC_SIZE];
    uint8_t actual_size;
    if (secureflash.flash_info.flash_profile->security_feature.RPMC) {
        if (secureflash_get_mc(&secureflash, mc_id, mc,
                               size, &actual_size, client_id) != 0) {
            return ETSS_ERR_STORAGE_FAILURE;
        }
        size = ETSS_UTILS_MIN(size, actual_size);
        etss_req_mngr_write(mc, size);
        return ETSS_SUCCESS;
    } else {
        return ETSS_ERR_NOT_SUPPORTED;
    }
}
