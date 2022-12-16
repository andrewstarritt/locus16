/* memory.cpp
 *
 * Locus 16 memory module, part of the Locus 16 Emulator.
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

#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

// Recall ROM is 0x8000 to 0x9000 and devices are 0x7000 to 0x7FFF
//
#define MEMORY_FIRST   (-28672)         // 0x9000 - inclusive
#define MEMORY_LAST    (+28672)         // 0x7000 - exclusive

using namespace L16E;

// Number of 4K byte / 2K word blocks (e.g. 0x9000 to 0xA000)
// is 10 for range 0x9000 to 0x3000 and 64 (4 by 16) for the
// range 0x3000 to 0x7000.
//
static const size_t blockSize = 0x1000;
static const size_t number = 10 + (4 * 16);   // 74
static const size_t totalSize = number * blockSize;

static const int noOffset = -1;

// As usual, lower address is inclusive, upper address is exclusive.
//
static const Int16 mapRegisterStart = 0x7B00;
static const Int16 mapRegisterEnd   = 0x7B00 + (2*MemoryMapper::maximumNumberOfMaps);

//==============================================================================
// MemoryController
//==============================================================================
//
MemoryMapper::MemoryMapper (DataBus* const dataBus) :
   DataBus::Device (dataBus, mapRegisterStart, mapRegisterEnd,
                    "Memory Controller", false)
{
   // Set default mappings and calc default offsets.
   //
   for (int slot = 0; slot < maximumNumberOfMaps; slot++) {
      this->mapValues[slot] = 0x0000;              // default
      this->offsets [slot][0x8] = noOffset;        // rom - no mapping
      this->offsets [slot][0x9] = 0 * blockSize;   // fixed
      this->offsets [slot][0xA] = 1 * blockSize;   // fixed
      this->offsets [slot][0xB] = 2 * blockSize;   // fixed
      this->offsets [slot][0xC] = 3 * blockSize;   // fixed
      this->offsets [slot][0xD] = 4 * blockSize;   // fixed
      this->offsets [slot][0xE] = 5 * blockSize;   // fixed
      this->offsets [slot][0xF] = 6 * blockSize;   // fixed
      this->offsets [slot][0x0] = 7 * blockSize;   // fixed
      this->offsets [slot][0x1] = 8 * blockSize;   // fixed
      this->offsets [slot][0x2] = 10 * blockSize;  // mapable
      this->offsets [slot][0x3] = 11 * blockSize;  // mapable
      this->offsets [slot][0x4] = 12 * blockSize;  // mapable
      this->offsets [slot][0x5] = 13 * blockSize;  // mapable
      this->offsets [slot][0x6] = 9 * blockSize;   // fixed
      this->offsets [slot][0x7] = noOffset;        // hardware - no mapping
   }
   this->activeIdentity = 0;
}

//------------------------------------------------------------------------------
//
MemoryMapper::~MemoryMapper() { }

//------------------------------------------------------------------------------
//
void MemoryMapper::setActiveIdentity(const int id)
{
   this->activeIdentity = id;
   if (this->activeIdentity < 0 || this->activeIdentity >= maximumNumberOfMaps) {
      std::cerr << "activeIdentity (" << id << ") out of range" << std::endl;
      this->activeIdentity = 0;
   }
}

//------------------------------------------------------------------------------
//
Int16 MemoryMapper::getWord(const Int16 addr) const
{
   const int slot = (addr - mapRegisterStart) >> 1;
   return this->mapValues[slot];
}

//------------------------------------------------------------------------------
//
void MemoryMapper::setWord(const Int16 addr,
                           const Int16 value)
{
   const int slot = (addr - mapRegisterStart) >> 1;
   this->mapValues[slot] = value;

   // For efficiency we pre-calculate stuff when the map word is defined.
   // We only need to recalculate for 2000, 3000, 4000 and 5000 ranges.
   //
   for (int j = 0; j < 4; j++) {
      int shift = (3 - j)*4;              // 12, 8, 4, 0
      int index = (value >> shift) & 15;  // 0 .. 15
      int block = 10 + (4 * index) + j;   // 10 is offset for mappable memory
      this->offsets[slot][2 + j] = block * blockSize;
   }
}

//------------------------------------------------------------------------------
//
int MemoryMapper::mapAddress (const Int16 addr) const
{
   const int slot = this->activeIdentity;

   // Isolate the MS nibble of the address and use this to access the offsets.
   //
   const int msAddrNib = (addr >> 12) & 15;    // 0 to 15
   const int offset = this->offsets[slot][msAddrNib];
   if (offset >= 0) {
       return offset + (addr & 0x0FFF);
   }

   std::cerr << "Seg Fault: Address out of range: 0x"
             << std::uppercase << std::hex << addr << std::endl;
   _exit (12);
}


//==============================================================================
// Memory
//==============================================================================
//
Memory::Memory (const int numberIn,
                MemoryMapper* controllerIn,
                DataBus* const dataBus) :
   DataBus::Device (dataBus, MEMORY_FIRST, MEMORY_LAST, "Memory", false),
   number(numberIn),
   controller(controllerIn)
{
   this->memory = malloc (totalSize);

   // Provide signed word and unsigned byte access equivilents.
   //
   this->bytePtr = reinterpret_cast <UInt8*> (this->memory);
   this->wordPtr = reinterpret_cast <Int16*> (this->memory);
}

//------------------------------------------------------------------------------
//
Memory::~Memory()
{
   free (this->memory);
}

//------------------------------------------------------------------------------
//
bool Memory::initialise ()
{
   const int numberWords = totalSize/2;

   for (int paddr = 0; paddr < numberWords; paddr++) {
      this->wordPtr [paddr] = 0;
   }

   return true;
}

//------------------------------------------------------------------------------
//
UInt8 Memory::getByte(const Int16 addr) const
{
   const int paddr = this->controller->mapAddress(addr);
   return this->bytePtr[paddr];
}

//------------------------------------------------------------------------------
//
void Memory::setByte(const Int16 addr, const UInt8 value)
{
   const int paddr = this->controller->mapAddress(addr);
   this->bytePtr[paddr] = value;
}

//------------------------------------------------------------------------------
//
Int16 Memory::getWord(const Int16 addr) const
{
   // Locus 16 is big endian - we (Intel) are little endian.
   //
   const int paddr = this->controller->mapAddress(addr);
   return __builtin_bswap16 (this->wordPtr [paddr >> 1]);
}

//------------------------------------------------------------------------------
//
void Memory::setWord(const Int16 addr, const Int16  value)
{
   // Locus 16 is big endian.
   //
   const int paddr = this->controller->mapAddress(addr);
   this->wordPtr [paddr >> 1] = __builtin_bswap16 (value);
}

// end
