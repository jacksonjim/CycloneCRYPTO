/**
 * @file keccak.h
 * @brief Keccak sponge function
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

#ifndef _KECCAK_H
#define _KECCAK_H

//Dependencies
#include "core/crypto.h"

//The binary logarithm of the lane size
#ifndef KECCAK_L
   #define KECCAK_L 6
#endif

//Check lane size
#if (KECCAK_L == 3)
   //Base type that represents a lane
   typedef uint8_t keccak_lane_t;
   //Rotate left operation
   #define KECCAK_ROL(a, n) ROL8(a, (n) % 8)
   //String to lane conversion
   #define KECCAK_LOAD_LANE(p) p[0]
   //Lane to string conversion
   #define KECCAK_STORE_LANE(a, p) p[0] = a
#elif (KECCAK_L == 4)
   //Base type that represents a lane
   #define keccak_lane_t uint16_t
   //Rotate left operation
   #define KECCAK_ROL(a, n) ROL16(a, (n) % 16)
   //String to lane conversion
   #define KECCAK_LOAD_LANE(p) LOAD16LE(p)
   //Lane to string conversion
   #define KECCAK_STORE_LANE(a, p) STORE16LE(a, p)
#elif (KECCAK_L == 5)
   //Base type that represents a lane
   #define keccak_lane_t uint32_t
   //Rotate left operation
   #define KECCAK_ROL(a, n) ROL32(a, (n) % 32)
   //String to lane conversion
   #define KECCAK_LOAD_LANE(p) LOAD32LE(p)
   //Lane to string conversion
   #define KECCAK_STORE_LANE(a, p) STORE32LE(a, p)
#elif (KECCAK_L == 6)
   //Base type that represents a lane
   #define keccak_lane_t uint64_t
   //Rotate left operation
   #define KECCAK_ROL(a, n) ROL64(a, (n) % 64)
   //String to lane conversion
   #define KECCAK_LOAD_LANE(p) LOAD64LE(p)
   //Lane to string conversion
   #define KECCAK_STORE_LANE(a, p) STORE64LE(a, p)
#else
   #error KECCAK_L parameter is not valid
#endif

//The lane size of a Keccak-p permutation, in bits
#define KECCAK_W_BITS (1 << KECCAK_L)
//The lane size of a Keccak-p permutation, in bytes
#define KECCAK_W_BYTES ((1 << KECCAK_L) / 8)

//The width of a Keccak-p permutation, in bits
#define KECCAK_B_BITS (KECCAK_W_BITS * 25)
//The width of a Keccak-p permutation, in bytes
#define KECCAK_B_BYTES (KECCAK_W_BYTES * 25)

//The number of rounds for a Keccak-p permutation
#define KECCAK_NR (12 + 2 * KECCAK_L)

//Keccak padding byte
#define KECCAK_PAD 0x01
//SHA-3 padding byte
#define KECCAK_SHA3_PAD 0x06
//SHAKE padding byte
#define KECCAK_SHAKE_PAD 0x1F
//cSHAKE padding byte
#define KECCAK_CSHAKE_PAD 0x04

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Keccak context
 **/

typedef struct
{
   keccak_lane_t a[5][5];
   uint8_t buffer[24 * KECCAK_W_BYTES];
   uint_t blockSize;
   size_t length;
} KeccakContext;


//Keccak related functions
error_t keccakInit(KeccakContext *context, uint_t capacity);
void keccakAbsorb(KeccakContext *context, const void *input, size_t length);
void keccakFinal(KeccakContext *context, uint8_t pad);
void keccakSqueeze(KeccakContext *context, uint8_t *output, size_t length);
void keccakPermutBlock(KeccakContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
