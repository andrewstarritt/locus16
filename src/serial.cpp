/* serial.cpp
 *
 * Locus 16 Emulator serial module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2022-2024 Anddrew Starritt
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

#include "serial.h"
#include <iostream>

using namespace L16E;

//------------------------------------------------------------------------------
//
Serial::Serial (const Type typeIn,
                const Int16 statusRegisterAddressIn,
                DataBus* const dataBus) :
   DataBus::Device (dataBus, statusRegisterAddressIn, statusRegisterAddressIn+4,
                    "Serial", false),
   type(typeIn),
   statusRegisterAddress(statusRegisterAddressIn),
   dataRegisterAddress(statusRegisterAddressIn+2)
{
   this->peripheral = nullptr;
   this->bufferedByteExists = false;
   this->bufferedByte = 0;
}

//------------------------------------------------------------------------------
//
Serial::~Serial()  { }   // place holder

//------------------------------------------------------------------------------
//
void Serial::connect (Peripheral* peripheralIn)
{
   this->peripheral = peripheralIn;

   // We could be reconnecting
   this->bufferedByteExists = false;
   this->bufferedByte = 0;
}

//------------------------------------------------------------------------------
//
Int16 Serial::getWord(const Int16 addr) const
{
   Int16 result = 0x0000;

   if (addr == this->statusRegisterAddress) {
      if (this->peripheral) {
         if (this->type == Input) {
            if (!this->bufferedByteExists) {
               // Attempt to read a byte,
               //
               this->bufferedByteExists = this->peripheral->readByte (this->bufferedByte);
            }

            if (this->bufferedByteExists) {
               result = DataBus::XC000;    // ready to read
            } else {
               result = 0x0000;
            }

         } else {
            // Output always ready if a peripheral has been defined.
            //
            result = DataBus::XC000;
         }
      }

   } else if ((addr == this->dataRegisterAddress) &&
              (this->type == Input))
   {
      if (this->bufferedByteExists) {
         result = this->bufferedByte;
         this->bufferedByteExists = false;
      } else {
         result = DataBus::allOnes;
      }

   } else {
      // bogus/odd address or wrong type.
      // should we be able to read the lasy byte written??
      //
      std::cerr << "bogus address\n";
      result = DataBus::allOnes;
   }

   return result;
}

//------------------------------------------------------------------------------
//
void Serial::setWord(const Int16 addr, const Int16 value)
{
   if ((this->peripheral) &&  // sanity check
       (addr == this->dataRegisterAddress) &&
       (this->type == Output))
   {
      this->peripheral->writeByte(UInt8(value & 0xFF));
   }
}

// end

