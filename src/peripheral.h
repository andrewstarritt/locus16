/* peripheral.h
 */

#ifndef L16E_PERIPHERAL_H
#define L16E_PERIPHERAL_H

#include "locus16_common.h"

namespace L16E {

class Peripheral
{
public:
   explicit Peripheral(const char* name);
   ~Peripheral();

   virtual bool initialise();
   virtual bool readByte(UInt8& value);
   virtual bool writeByte(const UInt8 value);

protected:
   const char* const name;

   // Formatted perror function
   static void perrorf (const char* format, ...);

};

}

#endif // L16E_PERIPHERAL_H
