/**
 * @file stm32h5xx_crypto_hash.h
 * @brief STM32H5 hash hardware accelerator
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

#ifndef _STM32H5XX_CRYPTO_HASH_H
#define _STM32H5XX_CRYPTO_HASH_H

//Dependencies
#include "core/crypto.h"

//Hash hardware accelerator
#ifndef STM32H5XX_CRYPTO_HASH_SUPPORT
   #define STM32H5XX_CRYPTO_HASH_SUPPORT DISABLED
#elif (STM32H5XX_CRYPTO_HASH_SUPPORT != ENABLED && STM32H5XX_CRYPTO_HASH_SUPPORT != DISABLED)
   #error STM32H5XX_CRYPTO_HASH_SUPPORT parameter is not valid
#endif

//ALGO bitfield
#define HASH_CR_ALGO_SHA1       0
#define HASH_CR_ALGO_SHA224     HASH_CR_ALGO_1
#define HASH_CR_ALGO_SHA256     (HASH_CR_ALGO_1 | HASH_CR_ALGO_0)
#define HASH_CR_ALGO_SHA384     (HASH_CR_ALGO_3 | HASH_CR_ALGO_2)
#define HASH_CR_ALGO_SHA512     (HASH_CR_ALGO_3 | HASH_CR_ALGO_2 | HASH_CR_ALGO_1 | HASH_CR_ALGO_0)
#define HASH_CR_ALGO_SHA512_224 (HASH_CR_ALGO_3 | HASH_CR_ALGO_2 | HASH_CR_ALGO_0)
#define HASH_CR_ALGO_SHA512_256 (HASH_CR_ALGO_3 | HASH_CR_ALGO_2 | HASH_CR_ALGO_1)

//DATATYPE bitfield
#define HASH_CR_DATATYPE_32B 0
#define HASH_CR_DATATYPE_16B HASH_CR_DATATYPE_0
#define HASH_CR_DATATYPE_8B  HASH_CR_DATATYPE_1
#define HASH_CR_DATATYPE_1B  (HASH_CR_DATATYPE_1 | HASH_CR_DATATYPE_0)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Hash related functions
error_t hashInit(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
