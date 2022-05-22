/* rom.h
 *
 * Creates ROM from 0x8000 to 0x9000
 */

#ifndef L16E_ROM_H
#define L16E_ROM_H

#include "data_bus.h"

namespace L16E {

class ROM : public DataBus::Device {
public:
   explicit ROM(DataBus* const dataBus);
   virtual ~ROM();

   // Initialise rom form the the specified file.
   //
   void initialise(const char* romFile);

   UInt8 getByte(const Int16 addr) const;
   void setByte(const Int16 addr, const UInt8 value);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

private:
   UInt8* const romPtr;
   UInt8* bmem_ptr;
   Int16* wmem_ptr;
};

}

#endif // L16E_ROM_H
