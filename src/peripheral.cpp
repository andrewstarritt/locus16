/* peripheral.cpp
 */

#include "peripheral.h"
#include <string.h>
#include <stdarg.h>
#include <iostream>

using namespace L16E;


//------------------------------------------------------------------------------
// static
void Peripheral::perrorf (const char* format, ...)
{
   char message [200];
   va_list arguments;
   va_start (arguments, format);
   vsnprintf (message, sizeof (message), format, arguments);
   va_end (arguments);
   perror (message);
}

//------------------------------------------------------------------------------
//
Peripheral::Peripheral(const char* nameIn):
   name (strndup(nameIn, 40))
{
}

//------------------------------------------------------------------------------
//
Peripheral::~Peripheral()
{
}

//------------------------------------------------------------------------------
//
bool Peripheral::initialise()
{
   // Nothing do do here yet
   return true;
}

//------------------------------------------------------------------------------
//
bool Peripheral::readByte(UInt8& value)
{
   std::cerr << "Peripheral::readByte not implmented" << std::endl;
   return false;
}

//------------------------------------------------------------------------------
//
bool Peripheral::writeByte(const UInt8 value)
{
   std::cerr << "Peripheral::writeByte not implmented" << std::endl;
   return false;
}

// end
