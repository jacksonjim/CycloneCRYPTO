/**
 * @file aria.c
 * @brief ARIA encryption algorithm
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
 * @section Description
 *
 * ARIA is a 128-bit block cipher with 128-, 192-, and 256-bit keys. The
 * algorithm consists of a key scheduling part and data randomizing part.
 * Refer to RFC 5794 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.2
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL CRYPTO_TRACE_LEVEL

//Dependencies
#include "core/crypto.h"
#include "cipher/aria.h"
#include "debug.h"

//Check crypto library configuration
#if (ARIA_SUPPORT == ENABLED)

//Move operation
#define MOV128(b, a) \
{ \
   (b)[0] = (a)[0]; \
   (b)[1] = (a)[1]; \
   (b)[2] = (a)[2]; \
   (b)[3] = (a)[3]; \
}

//XOR operation
#define XOR128(b, a) \
{ \
   (b)[0] ^= (a)[0]; \
   (b)[1] ^= (a)[1]; \
   (b)[2] ^= (a)[2]; \
   (b)[3] ^= (a)[3]; \
}

//Rotate left operation
#define ROL128(b, a, n) \
{ \
   (b)[0] = ((a)[((n) / 32 + 0) % 4] << ((n) % 32)) | \
      ((a)[((n) / 32 + 1) % 4] >> (32 - ((n) % 32))); \
   (b)[1] = ((a)[((n) / 32 + 1) % 4] << ((n) % 32)) | \
      ((a)[((n) / 32 + 2) % 4] >> (32 - ((n) % 32))); \
   (b)[2] = ((a)[((n) / 32 + 2) % 4] << ((n) % 32)) | \
      ((a)[((n) / 32 + 3) % 4] >> (32 - ((n) % 32))); \
   (b)[3] = ((a)[((n) / 32 + 3) % 4] << ((n) % 32)) | \
      ((a)[((n) / 32 + 0) % 4] >> (32 - ((n) % 32))); \
}

//Byte access
#define X(n) ((x[(n) / 4] >> ((3 - ((n) % 4)) * 8)) & 0xFF)

//Substitution layer SL1
#define SL1(b, a) \
{ \
   uint32_t *x = (uint32_t *) (a); \
   uint32_t *y = (uint32_t *) (b); \
   y[0] = (uint32_t) sb1[X(0)] << 24; \
   y[0] |= (uint32_t) sb2[X(1)] << 16; \
   y[0] |= (uint32_t) sb3[X(2)] << 8; \
   y[0] |= (uint32_t) sb4[X(3)]; \
   y[1] = (uint32_t) sb1[X(4)] << 24; \
   y[1] |= (uint32_t) sb2[X(5)] << 16; \
   y[1] |= (uint32_t) sb3[X(6)] << 8; \
   y[1] |= (uint32_t) sb4[X(7)]; \
   y[2] = (uint32_t) sb1[X(8)] << 24; \
   y[2] |= (uint32_t) sb2[X(9)] << 16; \
   y[2] |= (uint32_t) sb3[X(10)] << 8; \
   y[2] |= (uint32_t) sb4[X(11)]; \
   y[3] = (uint32_t) sb1[X(12)] << 24; \
   y[3] |= (uint32_t) sb2[X(13)] << 16; \
   y[3] |= (uint32_t) sb3[X(14)] << 8; \
   y[3] |= (uint32_t) sb4[X(15)]; \
}

//Substitution layer SL2
#define SL2(b, a) \
{ \
   uint32_t *x = (uint32_t *) (a); \
   uint32_t *y = (uint32_t *) (b); \
   y[0] = (uint32_t) sb3[X(0)] << 24; \
   y[0] |= (uint32_t) sb4[X(1)] << 16; \
   y[0] |= (uint32_t) sb1[X(2)] << 8; \
   y[0] |= (uint32_t) sb2[X(3)]; \
   y[1] = (uint32_t) sb3[X(4)] << 24; \
   y[1] |= (uint32_t) sb4[X(5)] << 16; \
   y[1] |= (uint32_t) sb1[X(6)] << 8; \
   y[1] |= (uint32_t) sb2[X(7)]; \
   y[2] = (uint32_t) sb3[X(8)] << 24; \
   y[2] |= (uint32_t) sb4[X(9)] << 16; \
   y[2] |= (uint32_t) sb1[X(10)] << 8; \
   y[2] |= (uint32_t) sb2[X(11)]; \
   y[3] = (uint32_t) sb3[X(12)] << 24; \
   y[3] |= (uint32_t) sb4[X(13)] << 16; \
   y[3] |= (uint32_t) sb1[X(14)] << 8; \
   y[3] |= (uint32_t) sb2[X(15)]; \
}

//Diffusion layer
#define A(b, a) \
{ \
   uint32_t *x = (uint32_t *) (a); \
   uint32_t *y = (uint32_t *) (b); \
   y[0] = (X(3) ^ X(4) ^ X(6) ^ X(8) ^ X(9) ^ X(13) ^ X(14)) << 24; \
   y[0] |= (X(2) ^ X(5) ^ X(7) ^ X(8) ^ X(9) ^ X(12) ^ X(15)) << 16; \
   y[0] |= (X(1) ^ X(4) ^ X(6) ^ X(10) ^ X(11) ^ X(12) ^ X(15)) << 8; \
   y[0] |= (X(0) ^ X(5) ^ X(7) ^ X(10) ^ X(11) ^ X(13) ^ X(14)); \
   y[1] = (X(0) ^ X(2) ^ X(5) ^ X(8) ^ X(11) ^ X(14) ^ X(15)) << 24; \
   y[1] |= (X(1) ^ X(3) ^ X(4) ^ X(9) ^ X(10) ^ X(14) ^ X(15)) << 16; \
   y[1] |= (X(0) ^ X(2) ^ X(7) ^ X(9) ^ X(10) ^ X(12) ^ X(13)) << 8; \
   y[1] |= (X(1) ^ X(3) ^ X(6) ^ X(8) ^ X(11) ^ X(12) ^ X(13)); \
   y[2] = (X(0) ^ X(1) ^ X(4) ^ X(7) ^ X(10) ^ X(13) ^ X(15)) << 24; \
   y[2] |= (X(0) ^ X(1) ^ X(5) ^ X(6) ^ X(11) ^ X(12) ^ X(14)) << 16; \
   y[2] |= (X(2) ^ X(3) ^ X(5) ^ X(6) ^ X(8) ^ X(13) ^ X(15)) << 8; \
   y[2] |= (X(2) ^ X(3) ^ X(4) ^ X(7) ^ X(9) ^ X(12) ^ X(14)); \
   y[3] = (X(1) ^ X(2) ^ X(6) ^ X(7) ^ X(9) ^ X(11) ^ X(12)) << 24; \
   y[3] |= (X(0) ^ X(3) ^ X(6) ^ X(7) ^ X(8) ^ X(10) ^ X(13)) << 16; \
   y[3] |= (X(0) ^ X(3) ^ X(4) ^ X(5) ^ X(9) ^ X(11) ^ X(14)) << 8; \
   y[3] |= (X(1) ^ X(2) ^ X(4) ^ X(5) ^ X(8) ^ X(10) ^ X(15)); \
}

//S-box 1
static const uint8_t sb1[256] =
{
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
   0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
   0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
   0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
   0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
   0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
   0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
   0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
   0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
   0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
   0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

//S-box 2
static const uint8_t sb2[256] =
{
   0xE2, 0x4E, 0x54, 0xFC, 0x94, 0xC2, 0x4A, 0xCC, 0x62, 0x0D, 0x6A, 0x46, 0x3C, 0x4D, 0x8B, 0xD1,
   0x5E, 0xFA, 0x64, 0xCB, 0xB4, 0x97, 0xBE, 0x2B, 0xBC, 0x77, 0x2E, 0x03, 0xD3, 0x19, 0x59, 0xC1,
   0x1D, 0x06, 0x41, 0x6B, 0x55, 0xF0, 0x99, 0x69, 0xEA, 0x9C, 0x18, 0xAE, 0x63, 0xDF, 0xE7, 0xBB,
   0x00, 0x73, 0x66, 0xFB, 0x96, 0x4C, 0x85, 0xE4, 0x3A, 0x09, 0x45, 0xAA, 0x0F, 0xEE, 0x10, 0xEB,
   0x2D, 0x7F, 0xF4, 0x29, 0xAC, 0xCF, 0xAD, 0x91, 0x8D, 0x78, 0xC8, 0x95, 0xF9, 0x2F, 0xCE, 0xCD,
   0x08, 0x7A, 0x88, 0x38, 0x5C, 0x83, 0x2A, 0x28, 0x47, 0xDB, 0xB8, 0xC7, 0x93, 0xA4, 0x12, 0x53,
   0xFF, 0x87, 0x0E, 0x31, 0x36, 0x21, 0x58, 0x48, 0x01, 0x8E, 0x37, 0x74, 0x32, 0xCA, 0xE9, 0xB1,
   0xB7, 0xAB, 0x0C, 0xD7, 0xC4, 0x56, 0x42, 0x26, 0x07, 0x98, 0x60, 0xD9, 0xB6, 0xB9, 0x11, 0x40,
   0xEC, 0x20, 0x8C, 0xBD, 0xA0, 0xC9, 0x84, 0x04, 0x49, 0x23, 0xF1, 0x4F, 0x50, 0x1F, 0x13, 0xDC,
   0xD8, 0xC0, 0x9E, 0x57, 0xE3, 0xC3, 0x7B, 0x65, 0x3B, 0x02, 0x8F, 0x3E, 0xE8, 0x25, 0x92, 0xE5,
   0x15, 0xDD, 0xFD, 0x17, 0xA9, 0xBF, 0xD4, 0x9A, 0x7E, 0xC5, 0x39, 0x67, 0xFE, 0x76, 0x9D, 0x43,
   0xA7, 0xE1, 0xD0, 0xF5, 0x68, 0xF2, 0x1B, 0x34, 0x70, 0x05, 0xA3, 0x8A, 0xD5, 0x79, 0x86, 0xA8,
   0x30, 0xC6, 0x51, 0x4B, 0x1E, 0xA6, 0x27, 0xF6, 0x35, 0xD2, 0x6E, 0x24, 0x16, 0x82, 0x5F, 0xDA,
   0xE6, 0x75, 0xA2, 0xEF, 0x2C, 0xB2, 0x1C, 0x9F, 0x5D, 0x6F, 0x80, 0x0A, 0x72, 0x44, 0x9B, 0x6C,
   0x90, 0x0B, 0x5B, 0x33, 0x7D, 0x5A, 0x52, 0xF3, 0x61, 0xA1, 0xF7, 0xB0, 0xD6, 0x3F, 0x7C, 0x6D,
   0xED, 0x14, 0xE0, 0xA5, 0x3D, 0x22, 0xB3, 0xF8, 0x89, 0xDE, 0x71, 0x1A, 0xAF, 0xBA, 0xB5, 0x81
};

//S-box 3
static const uint8_t sb3[256] =
{
   0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
   0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
   0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
   0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
   0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
   0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
   0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
   0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
   0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
   0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
   0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
   0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
   0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
   0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
   0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
   0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

//S-box 4
static const uint8_t sb4[256] =
{
   0x30, 0x68, 0x99, 0x1B, 0x87, 0xB9, 0x21, 0x78, 0x50, 0x39, 0xDB, 0xE1, 0x72, 0x09, 0x62, 0x3C,
   0x3E, 0x7E, 0x5E, 0x8E, 0xF1, 0xA0, 0xCC, 0xA3, 0x2A, 0x1D, 0xFB, 0xB6, 0xD6, 0x20, 0xC4, 0x8D,
   0x81, 0x65, 0xF5, 0x89, 0xCB, 0x9D, 0x77, 0xC6, 0x57, 0x43, 0x56, 0x17, 0xD4, 0x40, 0x1A, 0x4D,
   0xC0, 0x63, 0x6C, 0xE3, 0xB7, 0xC8, 0x64, 0x6A, 0x53, 0xAA, 0x38, 0x98, 0x0C, 0xF4, 0x9B, 0xED,
   0x7F, 0x22, 0x76, 0xAF, 0xDD, 0x3A, 0x0B, 0x58, 0x67, 0x88, 0x06, 0xC3, 0x35, 0x0D, 0x01, 0x8B,
   0x8C, 0xC2, 0xE6, 0x5F, 0x02, 0x24, 0x75, 0x93, 0x66, 0x1E, 0xE5, 0xE2, 0x54, 0xD8, 0x10, 0xCE,
   0x7A, 0xE8, 0x08, 0x2C, 0x12, 0x97, 0x32, 0xAB, 0xB4, 0x27, 0x0A, 0x23, 0xDF, 0xEF, 0xCA, 0xD9,
   0xB8, 0xFA, 0xDC, 0x31, 0x6B, 0xD1, 0xAD, 0x19, 0x49, 0xBD, 0x51, 0x96, 0xEE, 0xE4, 0xA8, 0x41,
   0xDA, 0xFF, 0xCD, 0x55, 0x86, 0x36, 0xBE, 0x61, 0x52, 0xF8, 0xBB, 0x0E, 0x82, 0x48, 0x69, 0x9A,
   0xE0, 0x47, 0x9E, 0x5C, 0x04, 0x4B, 0x34, 0x15, 0x79, 0x26, 0xA7, 0xDE, 0x29, 0xAE, 0x92, 0xD7,
   0x84, 0xE9, 0xD2, 0xBA, 0x5D, 0xF3, 0xC5, 0xB0, 0xBF, 0xA4, 0x3B, 0x71, 0x44, 0x46, 0x2B, 0xFC,
   0xEB, 0x6F, 0xD5, 0xF6, 0x14, 0xFE, 0x7C, 0x70, 0x5A, 0x7D, 0xFD, 0x2F, 0x18, 0x83, 0x16, 0xA5,
   0x91, 0x1F, 0x05, 0x95, 0x74, 0xA9, 0xC1, 0x5B, 0x4A, 0x85, 0x6D, 0x13, 0x07, 0x4F, 0x4E, 0x45,
   0xB2, 0x0F, 0xC9, 0x1C, 0xA6, 0xBC, 0xEC, 0x73, 0x90, 0x7B, 0xCF, 0x59, 0x8F, 0xA1, 0xF9, 0x2D,
   0xF2, 0xB1, 0x00, 0x94, 0x37, 0x9F, 0xD0, 0x2E, 0x9C, 0x6E, 0x28, 0x3F, 0x80, 0xF0, 0x3D, 0xD3,
   0x25, 0x8A, 0xB5, 0xE7, 0x42, 0xB3, 0xC7, 0xEA, 0xF7, 0x4C, 0x11, 0x33, 0x03, 0xA2, 0xAC, 0x60
};

//Key scheduling constants
static const uint32_t c[12] =
{
   0x517CC1B7, 0x27220A94, 0xFE13ABE8, 0xFA9A6EE0, 0x6DB14ACC, 0x9E21C820,
   0xFF28B1D5, 0xEF5DE2B0, 0xDB92371D, 0x2126E970, 0x03249775, 0x04E8C90E
};

//ARIA128-ECB OID (1.2.410.200046.1.1.1)
const uint8_t ARIA128_ECB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x01};
//ARIA128-CBC OID (1.2.410.200046.1.1.2)
const uint8_t ARIA128_CBC_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x02};
//ARIA128-CFB OID (1.2.410.200046.1.1.3)
const uint8_t ARIA128_CFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x03};
//ARIA128-OFB OID (1.2.410.200046.1.1.4)
const uint8_t ARIA128_OFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x04};
//ARIA128-CTR OID (1.2.410.200046.1.1.5)
const uint8_t ARIA128_CTR_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x05};

//ARIA192-ECB OID (1.2.410.200046.1.1.6)
const uint8_t ARIA192_ECB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x06};
//ARIA192-CBC OID (1.2.410.200046.1.1.7)
const uint8_t ARIA192_CBC_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x07};
//ARIA192-CFB OID (1.2.410.200046.1.1.8)
const uint8_t ARIA192_CFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x08};
//ARIA192-OFB OID (1.2.410.200046.1.1.9)
const uint8_t ARIA192_OFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x09};
//ARIA192-CTR OID (1.2.410.200046.1.1.10)
const uint8_t ARIA192_CTR_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0A};

//ARIA256-ECB OID (1.2.410.200046.1.1.11)
const uint8_t ARIA256_ECB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0B};
//ARIA256-CBC OID (1.2.410.200046.1.1.12)
const uint8_t ARIA256_CBC_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0C};
//ARIA256-CFB OID (1.2.410.200046.1.1.13)
const uint8_t ARIA256_CFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0D};
//ARIA256-OFB OID (1.2.410.200046.1.1.14)
const uint8_t ARIA256_OFB_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0E};
//ARIA256-CTR OID (1.2.410.200046.1.1.15)
const uint8_t ARIA256_CTR_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x0F};

//ARIA128-GCM OID (1.2.410.200046.1.1.34)
const uint8_t ARIA128_GCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x22};
//ARIA192-GCM OID (1.2.410.200046.1.1.35)
const uint8_t ARIA192_GCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x23};
//ARIA256-GCM OID (1.2.410.200046.1.1.36)
const uint8_t ARIA256_GCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x24};

//ARIA128-CCM OID (1.2.410.200046.1.1.37)
const uint8_t ARIA128_CCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x25};
//ARIA192-CCM OID (1.2.410.200046.1.1.38)
const uint8_t ARIA192_CCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x26};
//ARIA256-CCM OID (1.2.410.200046.1.1.39)
const uint8_t ARIA256_CCM_OID[9] = {0x2A, 0x83, 0x1A, 0x8C, 0x9A, 0x6E, 0x01, 0x01, 0x27};

//Common interface for encryption algorithms
const CipherAlgo ariaCipherAlgo =
{
   "ARIA",
   sizeof(AriaContext),
   CIPHER_ALGO_TYPE_BLOCK,
   ARIA_BLOCK_SIZE,
   (CipherAlgoInit) ariaInit,
   NULL,
   NULL,
   (CipherAlgoEncryptBlock) ariaEncryptBlock,
   (CipherAlgoDecryptBlock) ariaDecryptBlock,
   (CipherAlgoDeinit) ariaDeinit
};


/**
 * @brief Odd round function
 * @param[in,out] d 128-bit string
 * @param[in] rk 128-bit string
 **/

static void OF(uint32_t *d, const uint32_t *rk)
{
   uint32_t t[4];

   //XOR D with RK
   XOR128(d, rk);
   //Substitution layer SL1
   SL1(t, d);
   //Diffusion layer
   A(d, t);
}


/**
 * @brief Even round function
 * @param[in,out] d 128-bit string
 * @param[in] rk 128-bit string
 **/

static void EF(uint32_t *d, const uint32_t *rk)
{
   uint32_t t[4];

   //XOR D with RK
   XOR128(d, rk);
   //Substitution layer SL2
   SL2(t, d);
   //Diffusion layer
   A(d, t);
}


/**
 * @brief Initialize a ARIA context using the supplied key
 * @param[in] context Pointer to the ARIA context to initialize
 * @param[in] key Pointer to the key
 * @param[in] keyLen Length of the key
 * @return Error code
 **/

error_t ariaInit(AriaContext *context, const uint8_t *key, size_t keyLen)
{
   uint_t i;
   uint32_t *ek;
   uint32_t *dk;
   const uint32_t *ck1;
   const uint32_t *ck2;
   const uint32_t *ck3;
   uint32_t w[16];

   //Check parameters
   if(context == NULL || key == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check the length of the master key
   if(keyLen == 16)
   {
      //128-bit master keys require a total of 12 rounds
      context->nr = 12;

      //Select the relevant constants
      ck1 = c + 0;
      ck2 = c + 4;
      ck3 = c + 8;
   }
   else if(keyLen == 24)
   {
      //192-bit master keys require a total of 14 rounds
      context->nr = 14;

      //Select the relevant constants
      ck1 = c + 4;
      ck2 = c + 8;
      ck3 = c + 0;
   }
   else if(keyLen == 32)
   {
      //256-bit master keys require a total of 16 rounds
      context->nr = 16;

      //Select the relevant constants
      ck1 = c + 8;
      ck2 = c + 0;
      ck3 = c + 4;
   }
   else
   {
      //Report an error
      return ERROR_INVALID_KEY_LENGTH;
   }

   //Determine the number of 32-bit words in the key
   keyLen /= 4;

   //Compute 128-bit values KL and KR
   for(i = 0; i < 16; i++)
   {
      if(i < keyLen)
      {
         w[i] = LOAD32BE(key + i * 4);
      }
      else
      {
         w[i] = 0;
      }
   }

   //Save KR
   MOV128(w + 8, w + 4);

   //Compute intermediate values W0, W1, W2, and W3
   MOV128(w + 4, w + 0);
   OF(w + 4, ck1);
   XOR128(w + 4, w + 8);

   MOV128(w + 8, w + 4);
   EF(w + 8, ck2);
   XOR128(w + 8, w + 0);

   MOV128(w + 12, w + 8);
   OF(w + 12, ck3);
   XOR128(w + 12, w + 4);

   //Point to the encryption round keys
   ek = context->ek;

   //Compute ek1, ..., ek17 as follow
   ROL128(ek + 0, w + 4, 109);
   XOR128(ek + 0, w + 0);
   ROL128(ek + 4, w + 8, 109);
   XOR128(ek + 4, w + 4);
   ROL128(ek + 8, w + 12, 109);
   XOR128(ek + 8, w + 8);
   ROL128(ek + 12, w + 0, 109);
   XOR128(ek + 12, w + 12);
   ROL128(ek + 16, w + 4, 97);
   XOR128(ek + 16, w + 0);
   ROL128(ek + 20, w + 8, 97);
   XOR128(ek + 20, w + 4);
   ROL128(ek + 24, w + 12, 97);
   XOR128(ek + 24, w + 8);
   ROL128(ek + 28, w + 0, 97);
   XOR128(ek + 28, w + 12);
   ROL128(ek + 32, w + 4, 61);
   XOR128(ek + 32, w + 0);
   ROL128(ek + 36, w + 8, 61);
   XOR128(ek + 36, w + 4);
   ROL128(ek + 40, w + 12, 61);
   XOR128(ek + 40, w + 8);
   ROL128(ek + 44, w + 0, 61);
   XOR128(ek + 44, w + 12);
   ROL128(ek + 48, w + 4, 31);
   XOR128(ek + 48, w + 0);
   ROL128(ek + 52, w + 8, 31);
   XOR128(ek + 52, w + 4);
   ROL128(ek + 56, w + 12, 31);
   XOR128(ek + 56, w + 8);
   ROL128(ek + 60, w + 0, 31);
   XOR128(ek + 60, w + 12);
   ROL128(ek + 64, w + 4, 19);
   XOR128(ek + 64, w + 0);

   //Decryption round keys are derived from the encryption round keys
   dk = context->dk;
   //Compute dk1
   MOV128(dk + 0, ek + context->nr * 4);

   //Compute dk2, ..., dk(n)
   for(i = 1; i < context->nr; i++)
   {
      A(dk + i * 4, ek + (context->nr - i) * 4);
   }

   //Compute dk(n + 1)
   MOV128(dk + i * 4, ek + 0);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Encrypt a 16-byte block using ARIA algorithm
 * @param[in] context Pointer to the ARIA context
 * @param[in] input Plaintext block to encrypt
 * @param[out] output Ciphertext block resulting from encryption
 **/

void ariaEncryptBlock(AriaContext *context, const uint8_t *input,
   uint8_t *output)
{
   uint32_t *ek;
   uint32_t p[4];
   uint32_t q[4];

   //Copy the plaintext to the buffer
   p[0] = LOAD32BE(input);
   p[1] = LOAD32BE(input + 4);
   p[2] = LOAD32BE(input + 8);
   p[3] = LOAD32BE(input + 12);

   //Point to the encryption round keys
   ek = context->ek;

   //Apply 11 rounds
   OF(p, ek + 0);
   EF(p, ek + 4);
   OF(p, ek + 8);
   EF(p, ek + 12);
   OF(p, ek + 16);
   EF(p, ek + 20);
   OF(p, ek + 24);
   EF(p, ek + 28);
   OF(p, ek + 32);
   EF(p, ek + 36);
   OF(p, ek + 40);

   //The number of rounds depends on the length of the master key
   if(context->nr == 12)
   {
      //128-bit master keys require a total of 12 rounds
      XOR128(p, ek + 44);
      SL2(q, p);
      XOR128(q, ek + 48);
   }
   else if(context->nr == 14)
   {
      //192-bit master keys require a total of 14 rounds
      EF(p, ek + 44);
      OF(p, ek + 48);
      XOR128(p, ek + 52);
      SL2(q, p);
      XOR128(q, ek + 56);
   }
   else
   {
      //256-bit master keys require a total of 16 rounds
      EF(p, ek + 44);
      OF(p, ek + 48);
      EF(p, ek + 52);
      OF(p, ek + 56);
      XOR128(p, ek + 60);
      SL2(q, p);
      XOR128(q, ek + 64);
   }

   //Copy the resulting ciphertext from the buffer
   STORE32BE(q[0], output);
   STORE32BE(q[1], output + 4);
   STORE32BE(q[2], output + 8);
   STORE32BE(q[3], output + 12);
}


/**
 * @brief Decrypt a 16-byte block using ARIA algorithm
 * @param[in] context Pointer to the ARIA context
 * @param[in] input Ciphertext block to decrypt
 * @param[out] output Plaintext block resulting from decryption
 **/

void ariaDecryptBlock(AriaContext *context, const uint8_t *input,
   uint8_t *output)
{
   uint32_t *dk;
   uint32_t p[4];
   uint32_t q[4];

   //Copy the ciphertext to the buffer
   p[0] = LOAD32BE(input);
   p[1] = LOAD32BE(input + 4);
   p[2] = LOAD32BE(input + 8);
   p[3] = LOAD32BE(input + 12);

   //Point to the decryption round keys
   dk = context->dk;

   //Apply 11 rounds
   OF(p, dk + 0);
   EF(p, dk + 4);
   OF(p, dk + 8);
   EF(p, dk + 12);
   OF(p, dk + 16);
   EF(p, dk + 20);
   OF(p, dk + 24);
   EF(p, dk + 28);
   OF(p, dk + 32);
   EF(p, dk + 36);
   OF(p, dk + 40);

   //The number of rounds depends on the length of the master key
   if(context->nr == 12)
   {
      //128-bit master keys require a total of 12 rounds
      XOR128(p, dk + 44);
      SL2(q, p);
      XOR128(q, dk + 48);
   }
   else if(context->nr == 14)
   {
      //192-bit master keys require a total of 14 rounds
      EF(p, dk + 44);
      OF(p, dk + 48);
      XOR128(p, dk + 52);
      SL2(q, p);
      XOR128(q, dk + 56);
   }
   else
   {
      //256-bit master keys require a total of 16 rounds
      EF(p, dk + 44);
      OF(p, dk + 48);
      EF(p, dk + 52);
      OF(p, dk + 56);
      XOR128(p, dk + 60);
      SL2(q, p);
      XOR128(q, dk + 64);
   }

   //The resulting value is the plaintext
   STORE32BE(q[0], output);
   STORE32BE(q[1], output + 4);
   STORE32BE(q[2], output + 8);
   STORE32BE(q[3], output + 12);
}


/**
 * @brief Release ARIA context
 * @param[in] context Pointer to the ARIA context
 **/

void ariaDeinit(AriaContext *context)
{
   //Clear ARIA context
   osMemset(context, 0, sizeof(AriaContext));
}

#endif
