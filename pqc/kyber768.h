/**
 * @file kyber768.h
 * @brief Kyber-768 KEM
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2023 Oryx Embedded SARL. All rights reserved.
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
 * @version 2.3.4
 **/

#ifndef _KYBER768_H
#define _KYBER768_H

//Dependencies
#include "core/crypto.h"

//Public key length
#define KYBER768_PUBLIC_KEY_LEN 1184
//Secret key length
#define KYBER768_SECRET_KEY_LEN 2400
//Ciphertext length
#define KYBER768_CIPHERTEXT_LEN 1088
//Shared secret length
#define KYBER768_SHARED_SECRET_LEN 32

//Common interface for key encapsulation mechanisms (KEM)
#define KYBER768_KEM_ALGO (&kyber768KemAlgo)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Kyber-768 related constants
extern const KemAlgo kyber768KemAlgo;

//Kyber-768 related functions
error_t kyber768GenerateKeyPair(const PrngAlgo *prngAlgo, void *prngContext,
   uint8_t *pk, uint8_t *sk);

error_t kyber768Encapsulate(const PrngAlgo *prngAlgo, void *prngContext,
   uint8_t *ct, uint8_t *ss, const uint8_t *pk);

error_t kyber768Decapsulate(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
