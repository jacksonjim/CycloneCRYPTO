/**
 * @file tm4c129_crypto_hash.c
 * @brief Tiva TM4C129 hash hardware accelerator
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

//Switch to the appropriate trace level
#define TRACE_LEVEL CRYPTO_TRACE_LEVEL

//Dependencies
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_shamd5.h"
#include "core/crypto.h"
#include "hardware/tm4c129/tm4c129_crypto.h"
#include "hardware/tm4c129/tm4c129_crypto_hash.h"
#include "hash/hash_algorithms.h"
#include "debug.h"

//Check crypto library configuration
#if (TM4C129_CRYPTO_HASH_SUPPORT == ENABLED)


/**
 * @brief Reset SHA/MD5 module
 **/

void hashReset(void)
{
   uint32_t temp;

   //Perform software reset
   SHAMD5_SYSCONFIG_R |= SHAMD5_SYSCONFIG_SOFTRESET;

   //Wait for the reset to complete
   while((SHAMD5_SYSSTATUS_R & SHAMD5_SYSSTATUS_RESETDONE) == 0)
   {
   }

   //Force idle mode
   temp = SHAMD5_SYSCONFIG_R & ~SHAMD5_SYSCONFIG_SIDLE_M;
   SHAMD5_SYSCONFIG_R = temp | SHAMD5_SYSCONFIG_SIDLE_FORCE;
}


/**
 * @brief Update hash value
 * @param[in] data Pointer to the input buffer
 * @param[in] length Length of the input buffer
 * @param[out] h Resulting hash value
 * @param[in] hLen Length of the hash value, in words
 **/

void hashProcessData(const uint8_t *data, size_t length, uint32_t *h,
   size_t hLen)
{
   size_t i;

   //Specify the length
   SHAMD5_LENGTH_R = length;

   //Digest input data
   while(length >= 64)
   {
      //Wait for the SHA/MD5 engine to be ready to accept data
      while((SHAMD5_IRQSTATUS_R & SHAMD5_IRQSTATUS_INPUT_READY) == 0)
      {
      }

      //Write 64-byte block
      SHAMD5_DATA_0_IN_R = LOAD32LE(data);
      SHAMD5_DATA_1_IN_R = LOAD32LE(data + 4);
      SHAMD5_DATA_2_IN_R = LOAD32LE(data + 8);
      SHAMD5_DATA_3_IN_R = LOAD32LE(data + 12);
      SHAMD5_DATA_4_IN_R = LOAD32LE(data + 16);
      SHAMD5_DATA_5_IN_R = LOAD32LE(data + 20);
      SHAMD5_DATA_6_IN_R = LOAD32LE(data + 24);
      SHAMD5_DATA_7_IN_R = LOAD32LE(data + 28);
      SHAMD5_DATA_8_IN_R = LOAD32LE(data + 32);
      SHAMD5_DATA_9_IN_R = LOAD32LE(data + 36);
      SHAMD5_DATA_10_IN_R = LOAD32LE(data + 40);
      SHAMD5_DATA_11_IN_R = LOAD32LE(data + 44);
      SHAMD5_DATA_12_IN_R = LOAD32LE(data + 48);
      SHAMD5_DATA_13_IN_R = LOAD32LE(data + 52);
      SHAMD5_DATA_14_IN_R = LOAD32LE(data + 56);
      SHAMD5_DATA_15_IN_R = LOAD32LE(data + 60);

      //Advance data pointer
      data += 64;
      length -= 64;
   }

   //Process final block
   if(length > 0)
   {
      //Wait for the SHA/MD5 engine to be ready to accept data
      while((SHAMD5_IRQSTATUS_R & SHAMD5_IRQSTATUS_INPUT_READY) == 0)
      {
      }

      //Write final block
      for(i = 0; i < length; i += 4)
      {
         //Write 32-bit word
         HWREG(SHAMD5_BASE + SHAMD5_O_DATA_0_IN + i) = LOAD32LE(data);
         //Advance data pointer
         data += 4;
      }
   }

   //Wait for the output to be ready
   while((SHAMD5_IRQSTATUS_R & SHAMD5_IRQSTATUS_OUTPUT_READY) == 0)
   {
   }

   //Read the resulting output value
   for(i = 0; i < hLen; i++)
   {
      h[i] = HWREG(SHAMD5_BASE + SHAMD5_O_IDIGEST_A + i * 4);
   }
}


#if (MD5_SUPPORT == ENABLED)

/**
 * @brief Digest a message using MD5
 * @param[in] data Pointer to the message being hashed
 * @param[in] length Length of the message
 * @param[out] digest Pointer to the calculated digest
 * @return Error code
 **/

error_t md5Compute(const void *data, size_t length, uint8_t *digest)
{
   uint_t i;
   uint32_t h[4];

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_MD5 | SHAMD5_MODE_ALGO_CONSTANT |
      SHAMD5_MODE_CLOSE_HASH;

   //Digest the message
   hashProcessData(data, length, h, MD5_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (MD5_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(h[i], digest + i * 4);
   }

   //Sucessful processing
   return NO_ERROR;
}


/**
 * @brief Initialize MD5 message digest context
 * @param[in] context Pointer to the MD5 context to initialize
 **/

void md5Init(Md5Context *context)
{
   //Set initial hash value
   context->h[0] = 0x67452301;
   context->h[1] = 0xEFCDAB89;
   context->h[2] = 0x98BADCFE;
   context->h[3] = 0x10325476;

   //Number of bytes in the buffer
   context->size = 0;
   //Total length of the message
   context->totalSize = 0;
}


/**
 * @brief Update the MD5 context with a portion of the message being hashed
 * @param[in] context Pointer to the MD5 context
 * @param[in] data Pointer to the buffer being hashed
 * @param[in] length Length of the buffer
 **/

void md5Update(Md5Context *context, const void *data, size_t length)
{
   size_t n;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_MD5;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         hashProcessData(data, n, context->h, MD5_DIGEST_SIZE / 4);

         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;
      }
      else
      {
         //The buffer can hold at most 64 bytes
         n = MIN(length, 64 - context->size);

         //Copy the data to the buffer
         osMemcpy(context->buffer + context->size, data, n);

         //Update the MD5 context
         context->size += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            hashProcessData(context->buffer, context->size, context->h,
               MD5_DIGEST_SIZE / 4);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Save the value of the SHA_DIGEST_COUNT register
   context->totalSize = SHAMD5_DIGEST_COUNT_R;

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);
}


/**
 * @brief Finish the MD5 message digest
 * @param[in] context Pointer to the MD5 context
 * @param[out] digest Calculated digest
 **/

void md5Final(Md5Context *context, uint8_t *digest)
{
   uint_t i;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_MD5 | SHAMD5_MODE_CLOSE_HASH;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Finish digest calculation
   hashProcessData(context->buffer, context->size, context->h,
      MD5_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (MD5_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}


/**
 * @brief Finish the MD5 message digest (no padding added)
 * @param[in] context Pointer to the MD5 context
 * @param[out] digest Calculated digest
 **/

void md5FinalRaw(Md5Context *context, uint8_t *digest)
{
   uint_t i;

   //Copy the resulting digest
   for(i = 0; i < (MD5_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}

#endif
#if (SHA1_SUPPORT == ENABLED)

/**
 * @brief Digest a message using SHA-1
 * @param[in] data Pointer to the message being hashed
 * @param[in] length Length of the message
 * @param[out] digest Pointer to the calculated digest
 * @return Error code
 **/

error_t sha1Compute(const void *data, size_t length, uint8_t *digest)
{
   uint_t i;
   uint32_t h[5];

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA1 | SHAMD5_MODE_ALGO_CONSTANT |
      SHAMD5_MODE_CLOSE_HASH;

   //Digest the message
   hashProcessData(data, length, h, SHA1_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA1_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(h[i], digest + i * 4);
   }

   //Sucessful processing
   return NO_ERROR;
}


/**
 * @brief Initialize SHA-1 message digest context
 * @param[in] context Pointer to the SHA-1 context to initialize
 **/

void sha1Init(Sha1Context *context)
{
   //Set initial hash value
   context->h[0] = BETOH32(0x67452301);
   context->h[1] = BETOH32(0xEFCDAB89);
   context->h[2] = BETOH32(0x98BADCFE);
   context->h[3] = BETOH32(0x10325476);
   context->h[4] = BETOH32(0xC3D2E1F0);

   //Number of bytes in the buffer
   context->size = 0;
   //Total length of the message
   context->totalSize = 0;
}


/**
 * @brief Update the SHA-1 context with a portion of the message being hashed
 * @param[in] context Pointer to the SHA-1 context
 * @param[in] data Pointer to the buffer being hashed
 * @param[in] length Length of the buffer
 **/

void sha1Update(Sha1Context *context, const void *data, size_t length)
{
   size_t n;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA1;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];
   SHAMD5_IDIGEST_E_R = context->h[4];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         hashProcessData(data, n, context->h, SHA1_DIGEST_SIZE / 4);

         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;
      }
      else
      {
         //The buffer can hold at most 64 bytes
         n = MIN(length, 64 - context->size);

         //Copy the data to the buffer
         osMemcpy(context->buffer + context->size, data, n);

         //Update the SHA-1 context
         context->size += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            hashProcessData(context->buffer, context->size, context->h,
               SHA1_DIGEST_SIZE / 4);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Save the value of the SHA_DIGEST_COUNT register
   context->totalSize = SHAMD5_DIGEST_COUNT_R;

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);
}


/**
 * @brief Finish the SHA-1 message digest
 * @param[in] context Pointer to the SHA-1 context
 * @param[out] digest Calculated digest
 **/

void sha1Final(Sha1Context *context, uint8_t *digest)
{
   uint_t i;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA1 | SHAMD5_MODE_CLOSE_HASH;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];
   SHAMD5_IDIGEST_E_R = context->h[4];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Finish digest calculation
   hashProcessData(context->buffer, context->size, context->h,
      SHA1_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA1_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}


/**
 * @brief Finish the SHA-1 message digest (no padding added)
 * @param[in] context Pointer to the SHA-1 context
 * @param[out] digest Calculated digest
 **/

void sha1FinalRaw(Sha1Context *context, uint8_t *digest)
{
   uint_t i;

   //Copy the resulting digest
   for(i = 0; i < (SHA1_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}

#endif
#if (SHA224_SUPPORT == ENABLED)

/**
 * @brief Digest a message using SHA-224
 * @param[in] data Pointer to the message being hashed
 * @param[in] length Length of the message
 * @param[out] digest Pointer to the calculated digest
 * @return Error code
 **/

error_t sha224Compute(const void *data, size_t length, uint8_t *digest)
{
   uint_t i;
   uint32_t h[7];

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA224 | SHAMD5_MODE_ALGO_CONSTANT |
      SHAMD5_MODE_CLOSE_HASH;

   //Digest the message
   hashProcessData(data, length, h, SHA224_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA224_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(h[i], digest + i * 4);
   }

   //Sucessful processing
   return NO_ERROR;
}


/**
 * @brief Initialize SHA-224 message digest context
 * @param[in] context Pointer to the SHA-224 context to initialize
 **/

void sha224Init(Sha224Context *context)
{
   //Set initial hash value
   context->h[0] = BETOH32(0xC1059ED8);
   context->h[1] = BETOH32(0x367CD507);
   context->h[2] = BETOH32(0x3070DD17);
   context->h[3] = BETOH32(0xF70E5939);
   context->h[4] = BETOH32(0xFFC00B31);
   context->h[5] = BETOH32(0x68581511);
   context->h[6] = BETOH32(0x64F98FA7);
   context->h[7] = BETOH32(0xBEFA4FA4);

   //Number of bytes in the buffer
   context->size = 0;
   //Total length of the message
   context->totalSize = 0;
}


/**
 * @brief Finish the SHA-224 message digest
 * @param[in] context Pointer to the SHA-224 context
 * @param[out] digest Calculated digest
 **/

void sha224Final(Sha224Context *context, uint8_t *digest)
{
   uint_t i;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA224 | SHAMD5_MODE_CLOSE_HASH;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];
   SHAMD5_IDIGEST_E_R = context->h[4];
   SHAMD5_IDIGEST_F_R = context->h[5];
   SHAMD5_IDIGEST_G_R = context->h[6];
   SHAMD5_IDIGEST_H_R = context->h[7];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Finish digest calculation
   hashProcessData(context->buffer, context->size, context->h,
      SHA224_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA224_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}

#endif
#if (SHA256_SUPPORT == ENABLED)

/**
 * @brief Digest a message using SHA-256
 * @param[in] data Pointer to the message being hashed
 * @param[in] length Length of the message
 * @param[out] digest Pointer to the calculated digest
 * @return Error code
 **/

error_t sha256Compute(const void *data, size_t length, uint8_t *digest)
{
   uint_t i;
   uint32_t h[8];

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA256 | SHAMD5_MODE_ALGO_CONSTANT |
      SHAMD5_MODE_CLOSE_HASH;

   //Digest the message
   hashProcessData(data, length, h, SHA256_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA256_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(h[i], digest + i * 4);
   }

   //Sucessful processing
   return NO_ERROR;
}


/**
 * @brief Initialize SHA-256 message digest context
 * @param[in] context Pointer to the SHA-256 context to initialize
 **/

void sha256Init(Sha256Context *context)
{
   //Set initial hash value
   context->h[0] = BETOH32(0x6A09E667);
   context->h[1] = BETOH32(0xBB67AE85);
   context->h[2] = BETOH32(0x3C6EF372);
   context->h[3] = BETOH32(0xA54FF53A);
   context->h[4] = BETOH32(0x510E527F);
   context->h[5] = BETOH32(0x9B05688C);
   context->h[6] = BETOH32(0x1F83D9AB);
   context->h[7] = BETOH32(0x5BE0CD19);

   //Number of bytes in the buffer
   context->size = 0;
   //Total length of the message
   context->totalSize = 0;
}


/**
 * @brief Update the SHA-256 context with a portion of the message being hashed
 * @param[in] context Pointer to the SHA-256 context
 * @param[in] data Pointer to the buffer being hashed
 * @param[in] length Length of the buffer
 **/

void sha256Update(Sha256Context *context, const void *data, size_t length)
{
   size_t n;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA256;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];
   SHAMD5_IDIGEST_E_R = context->h[4];
   SHAMD5_IDIGEST_F_R = context->h[5];
   SHAMD5_IDIGEST_G_R = context->h[6];
   SHAMD5_IDIGEST_H_R = context->h[7];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         hashProcessData(data, n, context->h, SHA256_DIGEST_SIZE / 4);

         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;
      }
      else
      {
         //The buffer can hold at most 64 bytes
         n = MIN(length, 64 - context->size);

         //Copy the data to the buffer
         osMemcpy(context->buffer + context->size, data, n);

         //Update the SHA-256 context
         context->size += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            hashProcessData(context->buffer, context->size, context->h,
               SHA256_DIGEST_SIZE / 4);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Save the value of the SHA_DIGEST_COUNT register
   context->totalSize = SHAMD5_DIGEST_COUNT_R;

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);
}


/**
 * @brief Finish the SHA-256 message digest
 * @param[in] context Pointer to the SHA-256 context
 * @param[out] digest Calculated digest
 **/

void sha256Final(Sha256Context *context, uint8_t *digest)
{
   uint_t i;

   //Acquire exclusive access to the SHA/MD5 module
   osAcquireMutex(&tm4c129CryptoMutex);

   //Reset the SHA/MD5 module before use
   hashReset();

   //Configure the SHA/MD5 module
   SHAMD5_MODE_R = SHAMD5_MODE_ALGO_SHA256 | SHAMD5_MODE_CLOSE_HASH;

   //Restore hash context
   SHAMD5_IDIGEST_A_R = context->h[0];
   SHAMD5_IDIGEST_B_R = context->h[1];
   SHAMD5_IDIGEST_C_R = context->h[2];
   SHAMD5_IDIGEST_D_R = context->h[3];
   SHAMD5_IDIGEST_E_R = context->h[4];
   SHAMD5_IDIGEST_F_R = context->h[5];
   SHAMD5_IDIGEST_G_R = context->h[6];
   SHAMD5_IDIGEST_H_R = context->h[7];

   //Restore the value of the SHA_DIGEST_COUNT register
   SHAMD5_DIGEST_COUNT_R = context->totalSize;

   //Finish digest calculation
   hashProcessData(context->buffer, context->size, context->h,
      SHA256_DIGEST_SIZE / 4);

   //Release exclusive access to the SHA/MD5 module
   osReleaseMutex(&tm4c129CryptoMutex);

   //Copy the resulting digest
   for(i = 0; i < (SHA256_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}


/**
 * @brief Finish the SHA-256 message digest (no padding added)
 * @param[in] context Pointer to the SHA-256 context
 * @param[out] digest Calculated digest
 **/

void sha256FinalRaw(Sha256Context *context, uint8_t *digest)
{
   uint_t i;

   //Copy the resulting digest
   for(i = 0; i < (SHA256_DIGEST_SIZE / 4); i++)
   {
      STORE32LE(context->h[i], digest + i * 4);
   }
}

#endif
#endif
