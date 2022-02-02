/**
 * @file s5d9_crypto_trng.h
 * @brief Synergy S5D9 true random number generator
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2022 Oryx Embedded SARL. All rights reserved.
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
 * @version 2.1.4
 **/

#ifndef _S5D9_CRYPTO_TRNG_H
#define _S5D9_CRYPTO_TRNG_H

//Dependencies
#include "core/crypto.h"

//True random number generator
#ifndef S5D9_CRYPTO_TRNG_SUPPORT
   #define S5D9_CRYPTO_TRNG_SUPPORT ENABLED
#elif (S5D9_CRYPTO_TRNG_SUPPORT != ENABLED && S5D9_CRYPTO_TRNG_SUPPORT != DISABLED)
   #error S5D9_CRYPTO_TRNG_SUPPORT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//TRNG related functions
error_t trngGetRandomData(uint8_t *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
