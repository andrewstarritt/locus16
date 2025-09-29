/* terminal.cpp
 *
 * Locus 16 Emulator terminal module, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#define DEBUG if (false)

using namespace L16E;

//------------------------------------------------------------------------------

using namespace L16E;


//------------------------------------------------------------------------------
//
Terminal::Terminal ():
   Peripheral("Terminal")
{
   this->pt_fd = -1;
   this->xt_fd = -1;
   this->ptname = "";
   this->xterm_pid = -1;
}

//------------------------------------------------------------------------------
//
Terminal::~Terminal()
{
   this->clear();
}

//------------------------------------------------------------------------------
//
void Terminal::clear()
{
   if (this->xt_fd >= 0) {
      close (this->xt_fd);
      this->xt_fd = -1;
   }

   if (this->pt_fd >= 0) {
      close (this->pt_fd);
      this->pt_fd = -1;
   }

   this->ptname = "";
   this->xterm_pid = -1;
}

//------------------------------------------------------------------------------
//
bool Terminal::initialise()
{
   static const char* clearScreen = "\033[2J";

   // From https://stackoverflow.com/questions/9996730/
   //      unix-c-open-new-terminal-and-redirect-output-to-it
   //
   // Open an unused pseudoterminal master device, returning a file
   // descriptor that can be used to refer to that device.
   //
   this->pt_fd = posix_openpt (O_RDWR);
   if (this->pt_fd == -1) {
      Peripheral::perrorf ("Could not open pseudo terminal");
      this->clear();
      return false;
   }
   DEBUG std::cout << "pt: " << this->pt_fd << "\n";

   // Return the name of the slave pseudo-terminal device corresponding
   // to the master referred to by pt_fd.
   //
   this->ptname = ptsname (this->pt_fd);
   if (!this->ptname) {
      Peripheral::perrorf ("Could not get pseudo terminal device name");
      this->clear();
      return false;
   }
   DEBUG std::cout << "ptname: " << this->ptname << "\n";

   // Unlocks the slave pseudo-terminal device corresponding to the master
   // pseudoterminal.
   //
   if (unlockpt (this->pt_fd) == -1) {
      Peripheral::perrorf ("Could not unlock terminal device %s", this->ptname);
      this->clear();
      return false;
   }

   // Create fork process to run xterm
   //
   this->xterm_pid = fork ();
   if (this->xterm_pid < 0) {
      Peripheral::perrorf ("Terminal::initialise fork() failure");
      this->clear();
      return false;
   }

   if (this->xterm_pid == 0) {
      // We are the child.
      //
      const char* const file = "xterm";
      char arg1 [20];
      snprintf(arg1, sizeof (arg1), "-S%s/%d", (strrchr (ptname, '/') + 1), pt_fd);

      char* argv [13];
      // strdup "converts" const char* to char*
      argv[0] = strdup(file);
      argv[1] = arg1;
      // black background, white text, larger font and a nice title.
      argv[2] = strdup("-bg");
      argv[3] = strdup("black");
      argv[4] = strdup("-fg");
      argv[5] = strdup("white");
      argv[6] = strdup("-fa");
      argv[7] = strdup("Monospace");
      argv[8] = strdup("-fs");
      argv[9] = strdup("10");
      argv[10] = strdup("-title");
      argv[11] = strdup("Locus 16 Emulator Terminal");
      argv[12] = NULL;

      DEBUG std::cout << "execvp: " << argv[0] << " " << argv[1] << "\n";

      execvp (file, argv);
      Peripheral::perrorf("Terminal::initialise execvp(%s) failed", file);
      return 127;
   }

   // Allocate the child process its own process group id (use its own pid).
   // This isolates it from from SIGINT (^C) sent to the main process.
   //
   int status = setpgid (this->xterm_pid, this->xterm_pid);
   if (status < 0) {
      Peripheral::perrorf ("setpgid()  %d", status);
   }

   // We are the parent - let child do it's stuff
   // Really need to do wait_pid to check all okay.
   //
   usleep (200*1000);  // 200 mSec

   // Regular file open
   //
   this->xt_fd = open(this->ptname, O_RDWR);
   if (this->xt_fd == -1) {
      Peripheral::perrorf ("Could not open %s", this->ptname);
      this->clear();
      return false;
   }
   DEBUG std::cout << "xterm fd " << this->xt_fd << "\n";


   char reply[40];
   char message[40];
   size_t size;
   ssize_t number;

   // Read and throw away the first input - it is the window id number.
   //
   number = read (this->xt_fd, reply, sizeof (reply));

   // Clear the terminal screen and send introductory message.
   //
   size = snprintf (message, sizeof (message), "%s", clearScreen);
   number = write (this->xt_fd, message, size);

   // Successfully opened terminal file - set non-blocking.
   //
   int flags;
   flags = fcntl (this->xt_fd, F_GETFL, 0);
   flags |= O_NONBLOCK;
   flags = fcntl (this->xt_fd, F_SETFL, flags);

   return (number > 0);
}

//------------------------------------------------------------------------------
//
bool Terminal::readByte (UInt8& value)
{
   bool result;

   if (this->xt_fd >= 0) {
      ssize_t number;
      number = read (this->xt_fd, &value, 1);
      if (number == 0) {
         // number = 0 implies end of input.
         //
         close(this->xt_fd);
         this->xt_fd = -1;
      }

      if (number < 0) {
         if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            /* This is an actual error
             */
            this->perrorf ("Terminal::readByte()");
         }
      }

      result = (number == 1);
   } else {
      result = false;
   }

   return result;
}

//------------------------------------------------------------------------------
//
bool Terminal::writeByte(const UInt8 value)
{
   bool result;

   if (this->xt_fd >= 0) {
      ssize_t number;
      number = write (this->xt_fd, &value, 1);

      if (number < 0) {
         if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            /* This is an actual error
             */
            this->perrorf ("Terminal::writeByte()");
         }
      }

      result = (number == 1);
   } else {
      result = false;
   }

   return result;
}

// end
