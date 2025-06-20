/**
 * @file rx65n_crypto_hash.c
 * @brief RX65N hash hardware accelerator
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
#include "r_tsip_rx_if.h"
#include "core/crypto.h"
#include "hardware/rx65n/rx65n_crypto.h"
#include "hardware/rx65n/rx65n_crypto_hash.h"
#include "hash/hash_algorithms.h"
#include "debug.h"

//Check crypto library configuration
#if (RX65N_CRYPTO_HASH_SUPPORT == ENABLED)

//Padding string
static const uint8_t padding[64] =
{
   0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//Forward declaration of functions
e_tsip_err_t R_TSIP_Md5GenerateMessageDigestSub(const uint32_t *hashIn,
   const uint32_t *data, uint32_t length, uint32_t *hashOut);

e_tsip_err_t R_TSIP_Sha1GenerateMessageDigestSub(const uint32_t *hashIn,
   const uint32_t *data, uint32_t length, uint32_t *hashOut);

e_tsip_err_t R_TSIP_Sha224256GenerateMessageDigestSub(const uint32_t *hashIn,
   const uint32_t *data, uint32_t length, uint32_t *hashOut);


#if (MD5_SUPPORT == ENABLED)

/**
 * @brief Initialize MD5 message digest context
 * @param[in] context Pointer to the MD5 context to initialize
 **/

void md5Init(Md5Context *context)
{
   //Set initial hash value
   context->h[0] = BETOH32(0x67452301);
   context->h[1] = BETOH32(0xEFCDAB89);
   context->h[2] = BETOH32(0x98BADCFE);
   context->h[3] = BETOH32(0x10325476);

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

   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         R_TSIP_Md5GenerateMessageDigestSub(context->h, data, n / 4,
            context->h);

         //Update the MD5 context
         context->totalSize += n;
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
         context->totalSize += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            R_TSIP_Md5GenerateMessageDigestSub(context->h, context->x, 16,
               context->h);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}


/**
 * @brief Finish the MD5 message digest
 * @param[in] context Pointer to the MD5 context
 * @param[out] digest Calculated digest
 **/

void md5Final(Md5Context *context, uint8_t *digest)
{
   uint_t i;
   size_t paddingSize;
   uint64_t totalSize;

   //Length of the original message (before padding)
   totalSize = context->totalSize * 8;

   //Pad the message so that its length is congruent to 56 modulo 64
   if(context->size < 56)
   {
      paddingSize = 56 - context->size;
   }
   else
   {
      paddingSize = 64 + 56 - context->size;
   }

   //Append padding
   md5Update(context, padding, paddingSize);

   //Append the length of the original message
   for(i = 0; i < 8; i++)
   {
      context->buffer[56 + i] = totalSize & 0xFF;
      totalSize >>= 8;
   }

   //Calculate the message digest
   md5ProcessBlock(context);

   //Copy the resulting digest
   for(i = 0; i < (MD5_DIGEST_SIZE / 4); i++)
   {
      STORE32BE(context->h[i], digest + i * 4);
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
      STORE32BE(context->h[i], digest + i * 4);
   }
}


/**
 * @brief Process message in 16-word blocks
 * @param[in] context Pointer to the MD5 context
 **/

void md5ProcessBlock(Md5Context *context)
{
   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Accelerate MD5 inner compression loop
   R_TSIP_Md5GenerateMessageDigestSub(context->h, context->x, 16, context->h);

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}

#endif
#if (SHA1_SUPPORT == ENABLED)

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

   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         R_TSIP_Sha1GenerateMessageDigestSub(context->h, data, n / 4,
            context->h);

         //Update the SHA-1 context
         context->totalSize += n;
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
         context->totalSize += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            R_TSIP_Sha1GenerateMessageDigestSub(context->h, context->w, 16,
               context->h);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}


/**
 * @brief Finish the SHA-1 message digest
 * @param[in] context Pointer to the SHA-1 context
 * @param[out] digest Calculated digest
 **/

void sha1Final(Sha1Context *context, uint8_t *digest)
{
   uint_t i;
   size_t paddingSize;
   uint64_t totalSize;

   //Length of the original message (before padding)
   totalSize = context->totalSize * 8;

   //Pad the message so that its length is congruent to 56 modulo 64
   if(context->size < 56)
   {
      paddingSize = 56 - context->size;
   }
   else
   {
      paddingSize = 64 + 56 - context->size;
   }

   //Append padding
   sha1Update(context, padding, paddingSize);

   //Append the length of the original message
   for(i = 0; i < 8; i++)
   {
      context->buffer[63 - i] = totalSize & 0xFF;
      totalSize >>= 8;
   }

   //Calculate the message digest
   sha1ProcessBlock(context);

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


/**
 * @brief Process message in 16-word blocks
 * @param[in] context Pointer to the SHA-1 context
 **/

void sha1ProcessBlock(Sha1Context *context)
{
   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Accelerate SHA-1 inner compression loop
   R_TSIP_Sha1GenerateMessageDigestSub(context->h, context->w, 16, context->h);

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}

#endif
#if (SHA224_SUPPORT == ENABLED)

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

#endif
#if (SHA256_SUPPORT == ENABLED)

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

   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Process the incoming data
   while(length > 0)
   {
      //Check whether some data is pending in the buffer
      if(context->size == 0 && length >= 64)
      {
         //The length must be a multiple of 64 bytes
         n = length - (length % 64);

         //Update hash value
         R_TSIP_Sha224256GenerateMessageDigestSub(context->h, data, n / 4,
            context->h);

         //Update the SHA-256 context
         context->totalSize += n;
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
         context->totalSize += n;
         //Advance the data pointer
         data = (uint8_t *) data + n;
         //Remaining bytes to process
         length -= n;

         //Check whether the buffer is full
         if(context->size == 64)
         {
            //Update hash value
            R_TSIP_Sha224256GenerateMessageDigestSub(context->h, context->w, 16,
               context->h);

            //Empty the buffer
            context->size = 0;
         }
      }
   }

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}


/**
 * @brief Finish the SHA-256 message digest
 * @param[in] context Pointer to the SHA-256 context
 * @param[out] digest Calculated digest
 **/

void sha256Final(Sha256Context *context, uint8_t *digest)
{
   uint_t i;
   size_t paddingSize;
   uint64_t totalSize;

   //Length of the original message (before padding)
   totalSize = context->totalSize * 8;

   //Pad the message so that its length is congruent to 56 modulo 64
   if(context->size < 56)
   {
      paddingSize = 56 - context->size;
   }
   else
   {
      paddingSize = 64 + 56 - context->size;
   }

   //Append padding
   sha256Update(context, padding, paddingSize);

   //Append the length of the original message
   for(i = 0; i < 8; i++)
   {
      context->buffer[63 - i] = totalSize & 0xFF;
      totalSize >>= 8;
   }

   //Calculate the message digest
   sha256ProcessBlock(context);

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


/**
 * @brief Process message in 16-word blocks
 * @param[in] context Pointer to the SHA-256 context
 **/

void sha256ProcessBlock(Sha256Context *context)
{
   //Acquire exclusive access to the TSIP module
   osAcquireMutex(&rx65nCryptoMutex);

   //Accelerate SHA-256 inner compression loop
   R_TSIP_Sha224256GenerateMessageDigestSub(context->h, context->w, 16,
      context->h);

   //Release exclusive access to the TSIP module
   osReleaseMutex(&rx65nCryptoMutex);
}

#endif
#endif
