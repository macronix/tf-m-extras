/*
 * Copyright (c) 2020-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _SECURE_FLASH_H_
#define _SECURE_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include "include/secureflash_error.h"
#include "JEDEC_security_HAL/vendor_impl/vendor_secureflash_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_INFO_MAX_NUM        (0x10)
/**
 * \file secureflash.h
 *
 * \brief This file describes secure Flash API layer
 * \details Secure Flash API layer grants access to valid clients
 *          based on pre-provisioned application information.
 */
/** \defgroup secureflash_api secure Flash API layer
 * @{
 */
/*!
 * \struct app_data_t
 *
 * \brief Structure to store the pre-provisioned application & secure zone
 *        binding information.
 */
typedef struct{
    int32_t app_id;     /*!< The id of applications */
    uint32_t key_id;    /*!< The id of crypto root keys */
    uint32_t zone_id:8, /*!< The id of security zone id */
             mc_id:8,   /*!< The id of monotonic counter */
             reserved:16;
}app_data_t;

/*!
 * \struct app_info_t
 *
 * \brief Structure to store the pre-provisioned application information.
 */
typedef struct{
    uint8_t id;                            /*!< The id of app info */
    uint8_t num;                           /*!< The number of app_data items */
    app_data_t app_data[APP_INFO_MAX_NUM]; /*!< The detailed app_data */
}app_info_t;


typedef struct session_info_t session_info_t;

/*!
 * \struct session_info_t
 *
 * \brief Structure to store secure Flash session information.
 */
 struct session_info_t{
     uint32_t key_id;         /*!< Root key id */
     uint32_t session_key_id; /*!< Session key id */
     uint32_t session_id;     /*!< Session id */
};
/*!
 * \struct secureflash_t
 *
 * \brief Structure indicating secure Flash API layer informations
 */
typedef struct{
    //char *name; /*!< Secure Flash name */
    uint32_t _init_ref_count;       /*!< The initialization count of secure Flash */
    bool _is_initialized;           /*!< Secure Flash initialization status */
    app_info_t app_info;            /*!< The pre-provisioned application information of secure Flash */
    secure_flash_info_t flash_info; /*!< The specific secure Flash information */
    //struct sfdp_hdr_info *sfdp_info; /*!< Reserved for SFDP information */
}secureflash_t;

/**
 * \brief Initialize secure Flash.
 *
 * \param[in] secureflash  Secure Flash to initialize
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_init(secureflash_t *secureflash);

/**
 * \brief Uninitialize secure Flash.
 *
 * \param[in] secureflash  Secure Flash to deinitialize
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_uninit(secureflash_t *secureflash);
/* Provisioning Functionality */
#ifdef SECUREFLASH_PROVISION
/**
 * \brief Perform secure Flash provisioning based on provisioning data.
 *
 * \param[in] secureflash     Secure Flash to be provisioned
 * \param[in] provision_data  Provisioning data
 * \param[in] data_length     The size of provisioning data
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_provision(secureflash_t *secureflash,
                              uint8_t *provision_data, size_t data_length);
#endif
/* Deployment Functionality */

/**
 * \brief Read data from secure Flash.
 *
 * \param[in]  secureflash     Secure Flash to read
 * \param[out] buffer          buffer Buffer to store the data read from
 *                             secure Flash
 * \param[in]  addr            Target address of secure Flash
 * \param[in]  size            Size of data items to read
 * \param[in]  app_id          Application id
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_secure_read(secureflash_t *secureflash, void *buffer,
                                size_t addr, size_t size, int32_t app_id);
/**
 * \brief Program data to secure Flash.
 *
 * \param[in] secureflash      Secure Flash to program
 * \param[in] buffer           Pointer to a buffer containing data to be
 *                             programmed to secure Flash
 * \param[in] addr             Target address of secure Flash
 * \param[in] size             Size of data to program
 * \param[in] app_id           Application id
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_secure_program(secureflash_t *secureflash,
                                   const void *buffer, size_t addr,
                                   size_t size, int32_t app_id);
/**
 * \brief Erase secure Flash.
 *
 * \param[in] secureflash     Secure Flash to erase
 * \param[in] addr            Address of sector/block to be erased
 * \param[in] size            Size to be erased in bytes
 * \param[in] app_id          Application id
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_secure_erase(secureflash_t *secureflash, size_t addr,
                                 size_t size, int32_t app_id);
/**
 * \brief Get an unique value based on secure Flash Physical
 *        Unclonable Function.
 *
 * \param[in]  secureflash     Secure Flash to access
 * \param[out] puf             Pointer to a buffer storing derived value
 * \param[in]  size            Size of buffer
 * \param[out] actual_size     The actual size of derived value in bytes
 * \param[in]  input_data      Pointer to input data
 * \param[in]  input_data_size The size of input_data in bytes
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_get_puf(secureflash_t *secureflash, uint8_t *puf,
                            uint8_t size, uint8_t *actual_size,
                            uint8_t *input_data, uint8_t input_data_size);
/**
 * \brief Get a true random number from secure Flash True Random
 *        Number Generator.
 * \param[in]  secureflash      Secure Flash to access
 * \param[out] random           Pointer to a buffer storing derived random number
 * \param[in]  size             Size of buffer
 * \param[out] actual_size      The actual size of derived random number
 *                              in bytes
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_get_trng(secureflash_t *secureflash, uint8_t *random,
                             uint8_t size, uint8_t *actual_size);
/**
 * \brief Get the unique id of secure Flash.
 *
 * \param[in]  secureflash      Secure Flash to access
 * \param[out] uid              Pointer to a buffer storing derived unique id
 * \param[in]  size             Size of buffer
 * \param[out] actual_size      The actual size of derived unique id in bytes
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_get_uid(secureflash_t *secureflash, uint8_t *uid,
                            uint8_t size, uint8_t *actual_size);
/**
 * \brief Increment the given monotonic counter of secure Flash.
 *
 * \param[in] secureflash       Secure Flash to access
 * \param[in] mc_addr           Target monotonic counter
 * \param[in] app_id            Application id
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_increase_mc(secureflash_t *secureflash, uint8_t mc_addr,
                                int32_t app_id);
/**
 * \brief Read the given monotonic counter of secure Flash.
 *
 * \param[in]  secureflash      Secure Flash to access
 * \param[in]  mc_addr          Target monotonic counter
 * \param[out] mc               Pointer to a buffer storing counter value
 * \param[in]  size             Size of buffer in bytes
 * \param[out] actual_size      The actual size of counter value in bytes
 * \param[in]  app_id           Application id
 *
 * \return SECUREFLASH_SUCCESS if successful,
 *         or a specific SECUREFLASH_ERROR_XXX error code
 */
int32_t secureflash_get_mc(secureflash_t *secureflash, uint8_t mc_addr,
                           uint8_t *mc, uint8_t size,
                           uint8_t *actual_size, int32_t app_id);

/**@}*/
#ifdef __cplusplus
}
#endif
#endif /* _SECURE_FLASH_H_ */
