/**
 * @file esp32_c6_crypto_hash.h
 * @brief ESP32-C6 hash hardware accelerator
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneCRYPTO Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.2
 **/

#ifndef _ESP32_C6_CRYPTO_HASH_H
#define _ESP32_C6_CRYPTO_HASH_H

//Dependencies
#include "core/crypto.h"

//Hash hardware accelerator
#ifndef ESP32_C6_CRYPTO_HASH_SUPPORT
   #define ESP32_C6_CRYPTO_HASH_SUPPORT DISABLED
#elif (ESP32_C6_CRYPTO_HASH_SUPPORT != ENABLED && ESP32_C6_CRYPTO_HASH_SUPPORT != DISABLED)
   #error ESP32_C6_CRYPTO_HASH_SUPPORT parameter is not valid
#endif

//SHA Mode register
#define SHA_MODE_SHA1   0x00000000
#define SHA_MODE_SHA224 0x00000001
#define SHA_MODE_SHA256 0x00000002

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Hash related functions
void esp32c6ShaInit(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
