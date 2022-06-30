/**
 * @file gd32f4xx_crypto_trng.c
 * @brief GD32F4 true random number generator
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
 * @version 2.1.6
 **/

// Switch to the appropriate trace level
#define TRACE_LEVEL CRYPTO_TRACE_LEVEL

// Dependencies
#include "gd32f4xx.h"
#include "core/crypto.h"
#include "hardware/gd32f4xx/gd32f4xx_crypto.h"
#include "hardware/gd32f4xx/gd32f4xx_crypto_trng.h"
#include "debug.h"

// Check crypto library configuration
#if (GD32F4XX_CRYPTO_TRNG_SUPPORT == ENABLED)

/*!
    \brief      check whether the TRNG module is ready
    \param[in]  none
    \param[out] none
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus trng_ready_check(void)
{
   uint32_t timeout = 0;
   FlagStatus trng_flag = RESET;
   ErrStatus reval = SUCCESS;

   /* check wherther the random data is valid */
   do
   {
      timeout++;
      trng_flag = trng_flag_get(TRNG_FLAG_DRDY);
   } while ((RESET == trng_flag) && (0xFFFF > timeout));

   if (RESET == trng_flag)
   {
      /* ready check timeout */
      printf("Error: TRNG can't ready \r\n");
      trng_flag = trng_flag_get(TRNG_FLAG_CECS);
      printf("Clock error current status: %d \r\n", trng_flag);
      trng_flag = trng_flag_get(TRNG_FLAG_SECS);
      printf("Seed error current status: %d \r\n", trng_flag);
      reval = ERROR;
   }

   /* return check status */
   return reval;
}

/**
 * @brief TRNG module initialization
 * @return Error code
 **/

error_t trngInit(void)
{
   ErrStatus status = SUCCESS;
   uint8_t retry = 0;
   do
   {
      // Enable RNG peripheral clock
      rcu_periph_clock_enable(RCU_TRNG);

      // TRNG registers reset
      trng_deinit();
      trng_enable();

      /* check TRNG work status */
      status = trng_ready_check();
   } while (status == ERROR && ++retry < 3);

   // Return status code
   return  (status == SUCCESS) ? NO_ERROR : ERROR_FAILURE;
}

/**
 * @brief Get random data from the TRNG module
 * @param[out] data Buffer where to store random data
 * @param[in] length Number of random bytes to generate
 **/

error_t trngGetRandomData(uint8_t *data, size_t length)
{
   size_t i;
   uint32_t value;
   ErrStatus status = SUCCESS;

   // Acquire exclusive access to the RNG module
   osAcquireMutex(&gd32f4xxCryptoMutex);

   // Generate random data
   for (i = 0; i < length; i++)
   {
      // Generate a new 32-bit random value when necessary
      if ((i % 4) == 0)
      {
         // Get 32-bit random value
         if (SUCCESS == (status = trng_ready_check()))
         {
            value = trng_get_true_random_data();
         }
      }

      // Copy random byte
      data[i] = value & 0xFF;
      // Shift the 32-bit random value
      value >>= 8;
   }

   // Release exclusive access to the RNG module
   osReleaseMutex(&gd32f4xxCryptoMutex);

   // Return status code
   return (status == SUCCESS) ? NO_ERROR : ERROR_FAILURE;
}

#endif