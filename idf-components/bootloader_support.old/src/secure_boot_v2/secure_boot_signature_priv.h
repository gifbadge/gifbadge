/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/
#include "esp_secure_boot.h"
#include "rom/secure_boot.h"

esp_err_t verify_ecdsa_signature_block(const ets_secure_boot_signature_t *sig_block, const uint8_t *image_digest, const ets_secure_boot_sig_block_t *trusted_block);

esp_err_t verify_rsa_signature_block(const ets_secure_boot_signature_t *sig_block, const uint8_t *image_digest, const ets_secure_boot_sig_block_t *trusted_block);
