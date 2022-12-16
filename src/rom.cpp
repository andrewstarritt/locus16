/* rom.cpp
 *
 * Locus 16 ROM module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2021-2022 Anddrew Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The Locus 16 Emulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Locus 16 Emulator.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// We include ROM in memory here (for now)
//
#define ROM_FIRST   (-32768)         // 0x8000 - inclusive
#define ROM_LAST    (-28672)         // 0x9000 - exclusive
#define NUMBER_BYTES   (ROM_LAST - ROM_FIRST)

using namespace L16E;

ROM::ROM (const std::string romFileIn,
          DataBus* const dataBus) :
   DataBus::Device (dataBus, ROM_FIRST, ROM_LAST, "ROM", false),
   romFile (romFileIn),
   romPtr (reinterpret_cast <UInt8*> (malloc (NUMBER_BYTES)))
{
   // We consider addresses go from -32768 to -28672
   // Note: although C/C++ arrays start at zero, we can/are allowed to use
   // negative indices.
   //
   this->bmem_ptr = reinterpret_cast <UInt8*> (&this->romPtr[-(ROM_FIRST)]);
   this->wmem_ptr = reinterpret_cast <Int16*> (this->bmem_ptr);

   // First initialise all values to NULL.
   //
   for (Int16 addr = this->addrLow; addr < this->addrHigh; addr += 2) {
      this->setWord (addr, 0xFFFF);
   }
}

//------------------------------------------------------------------------------
//
ROM::~ROM() { }

//------------------------------------------------------------------------------
//
bool ROM::initialise()
{
   char message [80];
   int fd = open (this->romFile.c_str(), O_RDONLY);

   if (fd < 0) {
      snprintf(message, sizeof (message), "ROM open %s", this->romFile.c_str());
      perror(message);
      return false;
   }

   ssize_t size = read (fd, this->romPtr, NUMBER_BYTES);
   if (size <= 0) {
      snprintf(message, sizeof (message), "ROM read %s", this->romFile.c_str());
      perror(message);
   }

   printf("ROM %ld bytes loaded from %s\n", size, this->romFile.c_str());

   close (fd);

   return true;
}

//------------------------------------------------------------------------------
//
UInt8 ROM::getByte(const Int16 addr) const
{
   return this->bmem_ptr[addr];
}

//------------------------------------------------------------------------------
// It's ROM - we can't setByte.
//
void ROM::setByte(const Int16 addr, const UInt8 value) { }

//------------------------------------------------------------------------------
//
Int16 ROM::getWord(const Int16 addr) const
{
   // Locus 16 is big endian - we (Intel) are little endian.
   //
   return __builtin_bswap16 (this->wmem_ptr [addr >> 1]);
}

//------------------------------------------------------------------------------
// It's ROM - we can't setWord.
//
void ROM::setWord(const Int16 addr, const Int16  value) { }

// end
