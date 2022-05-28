# Locus 16 Emulator make file
#

.PHONY : all  clean  uninstall always

TOP=..

TARGET   = $(TOP)/locus16
OBJ_DIR  = $(TOP)/obj

# Options
#
CFLAGS += -Wall -std=gnu++11 -pipe -c -D_REENTRANT  -O3 -I.
LOPTS  += -l readline
LOPTS  += -l ncurses
LOPTS  += -l INIReader

RESOPTS += --input binary
RESOPTS += --output elf64-x86-64
RESOPTS += --binary-architecture i386

# All execute header files
#
HEADERS  = alp_processor.h
HEADERS += clock.h
HEADERS += data_bus.h
HEADERS += diagnostics.h
HEADERS += locus16_common.h
HEADERS += memory.h
HEADERS += peripheral.h
HEADERS += rom.h
HEADERS += serial.h
HEADERS += tape_punch.h
HEADERS += tape_reader.h
HEADERS += terminal.h

# Object files
#
OBJECTS  = $(OBJ_DIR)/build_datetime.o
OBJECTS += $(OBJ_DIR)/alp_processor.o
OBJECTS += $(OBJ_DIR)/clock.o
OBJECTS += $(OBJ_DIR)/data_bus.o
OBJECTS += $(OBJ_DIR)/diagnostics.o
OBJECTS += $(OBJ_DIR)/execute.o
OBJECTS += $(OBJ_DIR)/memory.o
OBJECTS += $(OBJ_DIR)/rom.o
OBJECTS += $(OBJ_DIR)/peripheral.o
OBJECTS += $(OBJ_DIR)/serial.o
OBJECTS += $(OBJ_DIR)/tape_punch.o
OBJECTS += $(OBJ_DIR)/tape_reader.o
OBJECTS += $(OBJ_DIR)/terminal.o
OBJECTS += $(OBJ_DIR)/main.o

# Resourses
#
OBJECTS += $(OBJ_DIR)/help_general.o
OBJECTS += $(OBJ_DIR)/help_usage.o
OBJECTS += $(OBJ_DIR)/LICENSE.o
OBJECTS += $(OBJ_DIR)/Warranty.o
OBJECTS += $(OBJ_DIR)/Redistribute.o
OBJECTS += $(OBJ_DIR)/Preamble.o

SENTINAL = $(OBJ_DIR)/.sentinal

all : $(TARGET)

$(TARGET) : $(OBJECTS)  Makefile
	@echo ""
	g++ -Wall -std=gnu++11 -pipe $(LOPTS) -o $(TARGET)  $(OBJECTS)
	@echo ""

build_datetime.cpp: always
	@echo "updating build_datetime.cpp"
	@echo '#include "build_datetime.h"'  > build_datetime.cpp
	@date -u '+std::string build_datetime () { return "%a %d %b %Y %H:%M:%S %Z"; }'  >> build_datetime.cpp

$(OBJ_DIR)/build_datetime.o : $(SENTINAL) build_datetime.cpp build_datetime.h Makefile
	g++ $(CFLAGS) -o $(OBJ_DIR)/build_datetime.o build_datetime.cpp

# General cpp file
# $< is source file, $@ is target file, % is wild card
#
$(OBJ_DIR)/%.o : %.cpp  %.h  peripheral.h  locus16_common.h data_bus.h $(SENTINAL) Makefile
	g++ $(CFLAGS) -o $@ $<

$(OBJ_DIR)/execute.o : execute.cpp execute.h $(HEADERS) $(SENTINAL) Makefile
	g++ $(CFLAGS) -o $(OBJ_DIR)/execute.o execute.cpp

$(OBJ_DIR)/main.o :  main.cpp build_datetime.h execute.h locus16_common.h $(SENTINAL) Makefile
	g++ $(CFLAGS) -o $(OBJ_DIR)/main.o main.cpp

# Resource files
#
# $< is source file, $@ is target file, % is wild card
#
$(OBJ_DIR)/%.o :  %.txt  $(SENTINAL) Makefile
	objcopy $(RESOPTS) $<  $@ 

# This is to make the directory, but dependency is on the sentinal file
#
$(SENTINAL):
	@echo "creating directory $(OBJ_DIR)" && \
	mkdir -p $(OBJ_DIR) && touch $(SENTINAL)

clean :
	rm -rf $(OBJ_DIR) *~

uninstall :
	rm -f $(TARGET)

# end
