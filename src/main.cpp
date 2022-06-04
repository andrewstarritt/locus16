/* locus16_main.cpp
 *
 * Locus 16 Emulator
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

#include <iostream>
#include <string>
#include <stdio.h>
#include <cinttypes>
#include <unistd.h>
#include <INIReader.h>

#include "execute.h"
#include "build_datetime.h"
#include "locus16_common.h"

//------------------------------------------------------------------------------
//
static void version (std::ostream& stream)
{
   stream << "Locus 16 Emulator Version " << LOCUS16_VERSION
          << "  Build " << build_datetime() << std::endl;
}

//------------------------------------------------------------------------------
// Macro to define functions to read and output contents of a resource text file.
//
#define PUT_RESOURCE(function, resource)                                  \
                                                                          \
static void function (std::ostream& stream)                               \
{                                                                         \
   extern char _binary_##resource##_start;                                \
   extern char _binary_##resource##_end;                                  \
                                                                          \
   const char* p = &_binary_##resource##_start;                           \
   while (p < &_binary_##resource##_end) stream << (*p++);                \
}


PUT_RESOURCE (help_general,  help_general_txt)
PUT_RESOURCE (help_usage,    help_usage_txt)
PUT_RESOURCE (help_license,  LICENSE_txt)
PUT_RESOURCE (help_warranty, Warranty_txt)
PUT_RESOURCE (help_redist,   Redistribute_txt)
PUT_RESOURCE (preamble,      Preamble_txt)

#undef PUT_RESOURCE

//------------------------------------------------------------------------------
//
int main(int argc, char** argv)
{
   std::string p1;
   std::string p2;

   // skip program name.
   //
   argc--;
   argv++;

   if (argc < 1) {
      std::cerr << "missing arguments" << std::endl;
      help_usage (std::cerr);
      return 1;
   }

   p1 = argv [0];

   if (p1 == "-h" || p1 == "--help") {
      version (std::cout);
      std::cout << std::endl;
      help_usage (std::cout);
      std::cout << std::endl;
      help_general(std::cout);
      return 0;
   }

   if (p1 == "-u" || p1 == "--usage") {
      help_usage (std::cout);
      return 0;
   }

   if (p1 == "-l" || p1 == "--license") {
      help_license (std::cout);
      return 0;
   }

   if (p1 == "-r" || p1 == "--redistribute") {
      help_redist (std::cout);
      return 0;
   }

   if (p1 == "-w" || p1 == "--warrant-y") {
      help_warranty (std::cout);
      return 0;
   }

   if (p1 == "-v" || p1 == "--version") {
      version (std::cout);
      return 0;
   }

   // Read actual program options and arguments.
   //
   // This slows the emulator down to approximatley real-time.
   // At least on my setup at home.
   //
   int sm = 26;   // default;

   if (p1 == "-s" || p1 == "--sleep") {
      if (argc >= 2) {

         int n = sscanf(argv [1], "%d", &sm);
         if (n != 1 || sm < 1) {
            std::cerr << "non integer or non positive sleep option value" << std::endl;
            return 1;
         }

         // Skip option and option value
         //
         argc -= 2;
         argv += 2;

      } else {
         std::cerr << "missing sleep option value" << std::endl;
         help_usage (std::cerr);
         return 1;
      }
   }

   if (argc < 1) {
      std::cerr << "missing arguments" << std::endl;
      help_usage (std::cerr);
      return 1;
   }
   p1 = argv [0];

   p2 = "punchout.txt";
   if (argc >= 2) {
      p2 = argv [1];
   }

   preamble (std::cout);
   std::cout << std::endl;

   version (std::cout);
   return run ("rom.dat", p1, p2, sm);
}

//------------------------------------------------------------------------------
//
int mainz () {
   INIReader* c = new INIReader("locus16.ini");

   int error = c->ParseError();
   if (error != 0) {
      printf ("parse error %d\n", error);
      return 2;
   }

   int number;
   number = c->GetInteger("Crate", "NumberSlots", -1);
   if (number < 1) {
      std::cout << "No slots specified" << "\n";
      return 1;
   }
   std::cout << "Number slots: " << number << "\n";

   for (int slot = 1; slot <= number; slot++) {
      char sectionText [20];

      snprintf(sectionText, sizeof(sectionText), "Slot%d", slot);

      const std::string kind = c->GetString(sectionText, "Kind", "None");
      std::cout << "slot: " << slot << "  kind: " << kind << "\n";

      if (kind == "ALP1") {
         const int p = c->GetInteger(sectionText, "Processor", -1);
         std::cout << "  processor no.: " << p << "\n";

         if (p < 1 || p > 2) {
            std::cerr << "  invalid processor number\n";
            error = MAX (4, error);
         }

      } else if (kind == "MemoryController") {
         // pass

      } else if (kind == "RAM") {
         // pass

      } else if (kind == "ROM") {
         // pass

      } else if (kind == "Terminal") {
         // pass

      } else if (kind == "None") {
         std::cout << "  warning - empty slot\n";
         error = MAX (1, error);

      } else {
         std::cerr << " unknown kind\n";
         error = MAX (4, error);
      }
   }

   if (error) {
      return error;
   }

   return 0;
}

// end
