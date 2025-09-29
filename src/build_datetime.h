/* build_datetime.h
 * 
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_BUILD_DATETIME_H
#define L16E_BUILD_DATETIME_H

#include <string>

// Returns the build datetime
// example format: Sat 05 Feb 2022-2025 09:02:58 UTC
// Note: build_datetime.cpp auto-generated at build time.
//
std::string build_datetime ();

# endif  // L16E_BUILD_DATETIME_H
