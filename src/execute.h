/* execute.h
 *
 * Run time execution manager, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_EXECUTE_H
#define L16E_EXECUTE_H

#include <string>

int run (const std::string iniFile,
         const std::string programFile,
         const std::string outputFile,
         const int sleepModulo);

#endif // L16E_EXECUTE_H
