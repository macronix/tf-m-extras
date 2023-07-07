/*
 * Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <string.h>
#include <stdlib.h>
#include "secureflash.h"
#include "psa_manifest/pid.h"
#include "TG424_3/JEDEC_security_HAL/include/error.h"
#include "TG424_3/vendor_impl/vendor_provisioning_impl.h"
#include "TG424_3/vendor_impl/vendor_secureflash.h"
#include "spi_nor_flash/spi_nor.h"
/**
 * \brief Query app info based on input address and app_id.
 *
 * \param[in] secureflash  Secure Flash context
 * \param[in] addr         Access address
 * \param[in] app_id       Application id
 *
 * \return Pointer to corresponding app_data,
 *         or null if there's no matching app_data
 */
static app_data_t *query_app_info(secureflash_t *secureflash,
                                  size_t addr, int32_t app_id)
{
    uint32_t n;
    uint32_t zone_id;

    zone_id = addr/(secureflash->flash_info.flash_profile->architecture.secure_zone_size);
    if (app_id == SP_ETSS) {
        for (n = 0; n < secureflash->app_info.num; n++) {
            if (secureflash->app_info.app_data[n].zone_id == zone_id) {
                return &(secureflash->app_info.app_data[n]);
            }
        }
    } else {
        for (n = 0; n < secureflash->app_info.num; n++) {
            if ((secureflash->app_info.app_data[n].app_id == app_id) &&
                (secureflash->app_info.app_data[n].zone_id == zone_id)) {
                return &(secureflash->app_info.app_data[n]);
            }
        }
    }
    return NULL;
}

/**
 * \brief Get app info which was provisioned during provisioning process.
 *
 * \param[in] secureflash  Secure Flash context
 *
 * \return SECUREFLASH_SUCCESS if success,
 *         or SECUREFLASH_ERROR_GET_PROVISION_INFO if fail
 */
static int32_t secureflash_get_app_info(secureflash_t *secureflash)
{
    uint32_t act_size;
    int32_t status;
    status = secureflash->flash_info.vendor_provisioning_op->provision_item_get_data(secureflash->flash_info.vendor_ctx,
                                                                                     ITEM_APP_INFO,
                                                                                     secureflash->app_info.app_data,
                                                                                     sizeof(secureflash->app_info.app_data),
                                                                                     &secureflash->app_info.num,
                                                                                     &act_size);
    if (status != SECUREFLASH_SUCCESS) {
        return SECUREFLASH_ERROR_GET_PROVISION_INFO;
    }
    return SECUREFLASH_SUCCESS;
}

/**
 * \brief Achieve specific secure Flash context, security operations, crypto
 *        wrappers based on secure Flash id.
 *
 * \param[in] secureflash  Secure Flash context
 * \param[in] id           Buffer for secure Flash id
 * \return SECUREFLASH_SUCCESS if success,
 *         or SECUREFLASH_ERROR_GET_PROVISION_INFO if fail
 */
static int32_t secureflash_match_id(secureflash_t *secureflash, uint8_t *id)
{
    uint32_t i;

    for (i = 0; i < sizeof(flash_info); i++) {
        if (!memcmp(id, flash_info[i]->id, flash_info[i]->id_len)) {
            jedec_set_vendor(flash_info[i]->vendor_security_op,
                             flash_info[i]->crypto_wrapper,
                             flash_info[i]->vendor_ctx);
            secureflash->flash_info.vendor_provisioning_op = flash_info[i]->vendor_provisioning_op;
            secureflash->flash_info.vendor_ctx = flash_info[i]->vendor_ctx;
            secureflash->flash_info.flash_profile = flash_info[i]->flash_profile;
            return SECUREFLASH_SUCCESS;
        }
    }
    return SECUREFLASH_ERROR_UNSUPPORTED_DEVICE;
}

/**
 * \brief Probe secure Flash via secure Flash id.
 *
 * \param[in] secureflash  Secure Flash context
 * \return SECUREFLASH_SUCCESS if success,
 *         or SECUREFLASH_ERROR_GET_PROVISION_INFO if fail
 */
static int32_t secureflash_probe(secureflash_t *secureflash)
{
    int32_t status;

    status = spi_nor_read_id(secureflash->flash_info.id, SECURE_FLASH_MAX_ID_LEN);
    if (status) {
        return SECUREFLASH_ERROR_DEVICE;
    }
    if (secureflash_match_id(secureflash, secureflash->flash_info.id) != SECUREFLASH_SUCCESS) {
        return SECUREFLASH_ERROR_UNSUPPORTED_DEVICE;
    }
    return SECUREFLASH_SUCCESS;
}


int32_t secureflash_init(secureflash_t *secureflash)
{
    int32_t status = SECUREFLASH_SUCCESS;

    if (!secureflash->_is_initialized) {
        secureflash->_init_ref_count = 0;
    }
    secureflash->_init_ref_count++;
    if (secureflash->_init_ref_count != 1) {
        goto init_exit_point;
    }
    /* spi nor init */
    status = spi_nor_init();
    if (status) {
        status = SECUREFLASH_ERROR_INITIAL;
        goto init_exit_point;
    }
    /* get secure flash info */
    status = secureflash_probe(secureflash);
    if (status) {
        goto init_exit_point;
    }
    /* get secure flash application info */
    memset(&secureflash->app_info, 0x00, sizeof(app_info_t));
    if (secureflash_get_app_info(secureflash) != SECUREFLASH_SUCCESS) {
        status = SECUREFLASH_ERROR_GET_PROVISION_INFO;
        goto init_exit_point;
    }
    /* jedec secure init */
    status = jedec_secure_init(SECUREFLASH_AUTHEN_KEY_ID);
    if (status != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_SECURE_INIT;
        goto init_exit_point;
    }
    secureflash->_is_initialized = true;
init_exit_point:
    return status;
}

int32_t secureflash_uninit(secureflash_t *secureflash)
{
    int32_t status = SECUREFLASH_SUCCESS;

    status = jedec_secure_uninit(SECUREFLASH_AUTHEN_KEY_ID);
    if (status != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_SECURE_DEINIT;
        goto uninit_exit_point;
    }
    memset(&secureflash->app_info, 0x00, sizeof(app_info_t));
    secureflash->_init_ref_count--;
    if (secureflash->_init_ref_count) {
        goto uninit_exit_point;
    }
    if (!secureflash->_is_initialized) {
        secureflash->_init_ref_count = 0;
        goto uninit_exit_point;
    }
    secureflash->_is_initialized = false;
uninit_exit_point:
    return status;
}

int32_t secureflash_secure_read(secureflash_t *secureflash, void *buffer,
                                size_t addr, size_t size, int32_t app_id)
{
    int32_t status = SECUREFLASH_SUCCESS;
    app_data_t *app_data;
    uint32_t session_key_id = 0;
    uint32_t read_len, offset, chunk, actual_read_size;
    uint8_t *buffer_ptr = buffer;

    if (!secureflash->_is_initialized) {
        return SECUREFLASH_ERROR_DEVICE;
    }
    if ((addr + size) > secureflash->flash_info.flash_profile->architecture.secure_zone_total_size) {
        return SECUREFLASH_ERROR_INVALID_ARGUMENT;
    }
    app_data = query_app_info(secureflash, addr, app_id);
    if (app_data == NULL) {
        status = SECUREFLASH_ERROR_ILLEGAL_ACCESS;
        goto secure_read_exit_point;
    }
    if (jedec_create_session(app_data->key_id, 0, &session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CREATE_SESSION;
        goto secure_read_exit_point;
    }
    read_len = secureflash->flash_info.flash_profile->architecture.secure_read_size;
    while (size > 0) {
        offset = addr % read_len;
        chunk = (offset + size < read_len) ? size : (read_len - offset);
        if (jedec_secure_read(addr, (uint8_t *)buffer_ptr, chunk,
                              session_key_id, &actual_read_size) != JEDEC_ERROR_NONE) {
            status = SECUREFLASH_ERROR_SECURE_READ;
            goto secure_read_exit_point;
        }
        size -= chunk;
        addr += chunk;
        buffer_ptr += chunk;
    }
    if (jedec_terminate_session(session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CLOSE_SESSION;
        goto secure_read_exit_point;
    }
secure_read_exit_point:
    return status;
}

int32_t secureflash_secure_program(secureflash_t *secureflash,
                                   const void *buffer, size_t addr,
                                   size_t size, int32_t app_id)
{
    int32_t status = SECUREFLASH_SUCCESS;
    app_data_t *app_data;
    uint32_t session_key_id = 0;
    uint32_t program_len, chunk, offset, actual_program_size;
    uint8_t *buffer_ptr = buffer;

    if (!secureflash->_is_initialized) {
        return SECUREFLASH_ERROR_DEVICE;
    }
    if ((addr + size) > secureflash->flash_info.flash_profile->architecture.secure_zone_total_size) {
        return SECUREFLASH_ERROR_INVALID_ARGUMENT;
    }
    app_data = query_app_info(secureflash, addr, app_id);
    if (app_data == NULL) {
        status = SECUREFLASH_ERROR_ILLEGAL_ACCESS;
        goto secure_program_exit_point;
    }
    if (jedec_create_session(app_data->key_id, 0, &session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CREATE_SESSION;
        goto secure_program_exit_point;
    }
    program_len = secureflash->flash_info.flash_profile->architecture.secure_program_size;
    while (size > 0) {
        offset = addr % program_len;
        chunk = (offset + size < program_len) ? size : (program_len - offset);
        if (jedec_secure_program(addr, (uint8_t *)buffer_ptr, chunk,
                                 session_key_id, &actual_program_size) != JEDEC_ERROR_NONE) {
            status = SECUREFLASH_ERROR_SECURE_PROGRAM;
            goto secure_program_exit_point;
        }
        size -= chunk;
        addr += chunk;
        buffer_ptr += chunk;
    }
    if (jedec_terminate_session(session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CLOSE_SESSION;
        goto secure_program_exit_point;
    }
secure_program_exit_point:
    return status;
}

int32_t secureflash_secure_erase(secureflash_t *secureflash, size_t addr,
                                 size_t size, int32_t app_id)
{
    int32_t status = SECUREFLASH_SUCCESS;
    app_data_t *app_data;
    uint32_t session_key_id = 0;
    uint32_t erase_size;

    if (!secureflash->_is_initialized) {
        return SECUREFLASH_ERROR_DEVICE;
    }
    if ((addr + size) > secureflash->flash_info.flash_profile->architecture.secure_zone_total_size) {
        return SECUREFLASH_ERROR_INVALID_ARGUMENT;
    }
    erase_size = secureflash->flash_info.flash_profile->architecture.regions_min_secure_erase_size;
    if (((addr % erase_size) != 0) ||
        (((addr + size) % erase_size) != 0)) {
        return SECUREFLASH_ERROR_INVALID_ARGUMENT;
    }
    app_data = query_app_info(secureflash, addr, app_id);
    if (app_data == NULL) {
        status = SECUREFLASH_ERROR_ILLEGAL_ACCESS;
        goto secure_erase_exit_point;
    }
    if (jedec_create_session(app_data->key_id, 0, &session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CREATE_SESSION;
        goto secure_erase_exit_point;
    }
    while (size > 0) {
        if (jedec_secure_erase(addr, erase_size, session_key_id) != JEDEC_ERROR_NONE) {
            status = SECUREFLASH_ERROR_SECURE_ERASE;
            goto secure_erase_exit_point;
        }
        size -= erase_size;
        addr += erase_size;
    }
    if (jedec_terminate_session(session_key_id) != JEDEC_ERROR_NONE) {
        status = SECUREFLASH_ERROR_CLOSE_SESSION;
        goto secure_erase_exit_point;
    }
secure_erase_exit_point:
    return status;
}

int32_t secureflash_get_puf(secureflash_t *secureflash, uint8_t *puf,
                            uint8_t size, uint8_t *actual_size,
                            uint8_t *input_data, uint8_t input_data_size)
{
    /* TODO */
    return SECUREFLASH_SUCCESS;
}

int32_t secureflash_get_trng(secureflash_t *secureflash, uint8_t *random,
                             uint8_t size, uint8_t *actual_size)
{
    /* TODO */
    return SECUREFLASH_SUCCESS;
}

int32_t secureflash_get_uid(secureflash_t *secureflash, uint8_t *uid,
                            uint8_t size, uint8_t *actual_size)
{
    /* TODO */
    return SECUREFLASH_SUCCESS;
}

static bool mc_access_grant(secureflash_t *secureflash, int32_t client_id,
                            uint8_t mc_addr, uint32_t *key_id)
{
    /* TODO */
    return false;
}

int32_t secureflash_increase_mc(secureflash_t *secureflash, uint8_t mc_addr,
                                int32_t app_id)
{
    /* TODO */
    return SECUREFLASH_SUCCESS;
}

int32_t secureflash_get_mc(secureflash_t *secureflash, uint8_t mc_addr,
                           uint8_t *mc, uint8_t size, uint8_t *actual_size,
                           int32_t app_id)
{
    /* TODO */
    return SECUREFLASH_SUCCESS;
}

int32_t secureflash_provision(secureflash_t *secureflash,
                              uint8_t *provision_data, size_t data_length)
{
    /* Directly call vendor specific secure flash provision implementation */
    return secureflash->flash_info.vendor_provisioning_op->perform_and_verify(secureflash->flash_info.vendor_ctx, provision_data, data_length);
}
