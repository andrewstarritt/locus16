/* tape_punch.cpp
 *
 * Tape punch module, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
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
TapePunch::TapePunch (const std::string filenameIn) :
   Peripheral("Tape Punch"),
   filename (filenameIn)
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
void TapePunch::setFilename (const std::string filenameIn)
{
   if (this->fd >= 0) {
      close(this->fd);
      this->fd = -1;
   }
   this->filename = filenameIn;
}

//------------------------------------------------------------------------------
//
bool TapePunch::initialise()
{
   this->fd = creat(this->filename.c_str(), 0644);

   if (this->fd >= 0) {
      // Successfully created/opened file - set non-blocking.
      //
      int flags;
      flags = fcntl (this->fd, F_GETFL, 0);
      flags |= O_NONBLOCK;
      flags = fcntl (this->fd, F_SETFL, flags);
   } else {
      this->perrorf("TapePunch::initialise (%s)", this->filename.c_str());
   }

   return (this->fd >= 0);
}

//------------------------------------------------------------------------------
//
bool TapePunch::writeByte (const UInt8 value)
{
   bool result;

   if (this->fd >= 0) {
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
