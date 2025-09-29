/* locus16_main.cpp
 *
 * Locus 16 Emulator
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include <iostream>
#include <string>
#include <stdio.h>
#include <cinttypes>
#include <unistd.h>

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
   return run ("locus16.ini", p1, p2, sm);
}

// end
