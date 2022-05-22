/* build_datetime.h
 * 
 * This file is part of the Locus 16 Emulator application.
 *
 * Copyright (c) 2021-2022  Andrew C. Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * The Locus 16 Emulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License and
 * the Lesser GNU General Public License along with the Locus 16 Emulator.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 */

#ifndef L16E_BUILD_DATETIME_H
#define L16E_BUILD_DATETIME_H

#include <string>

// Returns the build datetime
// example format: Sat 05 Feb 2022 09:02:58 UTC
// Note: build_datetime.cpp auto-generated at build time.
//
std::string build_datetime ();

# endif  // L16E_BUILD_DATETIME_H
