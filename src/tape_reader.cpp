/* tape_reader.cpp
 *
 * Tape reader module, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "tape_reader.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace L16E;

//------------------------------------------------------------------------------
//
TapeReader::TapeReader(const std::string filenameIn) :
   Peripheral("Tape Reader"),
   filename (filenameIn)
{
   this->fd = -1;
}

//------------------------------------------------------------------------------
//
TapeReader::~TapeReader()
{
   if (this->fd >= 0) {
      close(this->fd);
      this->fd = -1;
   }
}

//------------------------------------------------------------------------------
//
void TapeReader::setFilename (const std::string filenameIn)
{
   if (this->fd >= 0) {
      close(this->fd);
      this->fd = -1;
   }
   this->filename = filenameIn;
}

//------------------------------------------------------------------------------
//
bool TapeReader::initialise()
{
   this->fd = open(this->filename.c_str(), O_RDONLY);

   if (this->fd >= 0) {
      // Successfully opened file - set non-blocking.
      //
      int flags;
      flags = fcntl (this->fd, F_GETFL, 0);
      flags |= O_NONBLOCK;
      flags = fcntl (this->fd, F_SETFL, flags);
   } else {
      this->perrorf("TapeReader::initialise (%s)", this->filename.c_str());
   }

   return (this->fd >= 0);
}

//------------------------------------------------------------------------------
//
bool TapeReader::readByte(UInt8& value)
{
   bool result;

   if (this->fd >= 0) {
      ssize_t number;
      number = read (this->fd, &value, 1);
      if (number == 0) {
         // number = 0 implies end of input.
         //
         close(this->fd);
         this->fd = -1;
      }

      if (number < 0) {
         if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            /* This is an actual error
             */
            this->perrorf ("TapeReader::readByte()");
         }
      }

      result = (number == 1);
   } else {
      result = false;
   }

   return result;
}

// end
