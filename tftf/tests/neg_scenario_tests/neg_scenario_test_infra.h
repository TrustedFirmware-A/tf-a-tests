/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <firmware_image_package.h>

/*
    cert:
    cert_len:
    returnPtr: ptr to pubKey in cert

    return 0 upon success, -1 on fail
*/
int get_pubKey_from_cert(void *cert, size_t cert_len, void **returnPtr);
