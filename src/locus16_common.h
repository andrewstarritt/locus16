/* locus16_common.h
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_LOCUS_COMMON_H
#define L16E_LOCUS_COMMON_H

// Useful type neutral numerical macro fuctions.
//
#define ABS(a)             ((a) >= 0  ? (a) : -(a))
#define MIN(a, b)          ((a) <= (b) ? (a) : (b))
#define MAX(a, b)          ((a) >= (b) ? (a) : (b))
#define LIMIT(x,low,high)  (MAX(low, MIN(x, high)))

// Calculates number of items in an array.
//
#define ARRAY_LENGTH(xx)   (int (sizeof (xx) /sizeof (xx[0])))

#define LOCUS16_VERSION    "0.4.5"

typedef unsigned char   UInt8;
typedef short           Int16;

# endif  // L16E_LOCUS_COMMON_H
