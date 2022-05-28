/* tape_punch.cpp
 *
 * Tape punch module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2022  Andrew C. Starritt
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

#include "tape_punch.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace L16E;

//------------------------------------------------------------------------------
//
TapePunch::TapePunch(const char* filenameIn) :
   Peripheral("Tape Punch"),
   filename (strndup(filenameIn, 256))
{
   this->fd = -1;
}

//------------------------------------------------------------------------------
//
TapePunch::~TapePunch()
{
   if (this->fd >= 0) {
      close(this->fd);
      this->fd = -1;
   }
}

//------------------------------------------------------------------------------
//
bool TapePunch::initialise()
{
   this->fd = creat(this->filename, 0644);

   if (this->fd >= 0) {
      // Successfully created/opened file - set non-blocking.
      //
      int flags;
      flags = fcntl (this->fd, F_GETFL, 0);
      flags |= O_NONBLOCK;
      flags = fcntl (this->fd, F_SETFL, flags);
   } else {
      this->perrorf("TapePunch::initialise (%s)", this->filename);
   }

   return (this->fd >= 0);
}

//------------------------------------------------------------------------------
//
bool TapePunch::writeByte (const UInt8 value)
{
   bool result;

   if (this-fd >= 0) {
      ssize_t number;
      number = write (this->fd, &value, 1);

      if (number < 0) {
         if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            /* This is an actual error
             */
            this->perrorf ("TapePunch::writeByte()");
         }
      }

      result = (number == 1);
   } else {
      result = false;
   }

   return result;
}

// end
