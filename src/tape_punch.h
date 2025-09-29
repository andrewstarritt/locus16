/* tape_punch.h
 *
 * Tape punch module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2022-2025  Andrew C. Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License.
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
 */

#ifndef L16E_TAPE_PUNCH_H
#define L16E_TAPE_PUNCH_H

#include "locus16_common.h"
#include "peripheral.h"
#include <string>

namespace L16E {

class TapePunch : public Peripheral
{
public:
   explicit TapePunch(const std::string filename);
   ~TapePunch();

   void setFilename (const std::string filename);
   bool initialise();
   bool writeByte (const UInt8 value);

private:
   std::string filename;
   int fd;
};

}

#endif // L16E_TAPE_PUNCH_H
