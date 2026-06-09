APPLICATION_NAME = BareMetal
VERSION = V2

#  Directories
BUILD_DIR = Debug
BIN_DIR = bin
OBJ_DIR = obj

# All of the sources participating in the build are defined here
MAP_FILE = $(APPLICATION_NAME).map

# OPEN OCD
# Access the environment variable MY_FOLDER_PATH
OPENOCD_DIR := D:/Infineon board/Infineon/Tools/openocd
OCD = $(OPENOCD_DIR)
OCD_Scripts = "$(OPENOCD_DIR)/scripts"
OCD_Interface = "$(OPENOCD_DIR)/scripts/interface/kitprog3.cfg"
OCD_Target = "$(OPENOCD_DIR)/scripts/target/psoc4.cfg"

# Compiler
CC = arm-none-eabi-gcc
# Device specific flags [1]
DFLAGS = -mcpu=cortex-m0plus -mthumb -msoft-float
# Compiler flags 
CFLAGS = $(DFLAGS) -g -c -Wall -Wextra -std=gnu11 -O0
# Linker
LD = arm-none-eabi-gcc
# Path to linker script
LSCRIPT = ./../linker_script.ld
# Linker flags
LFLAGS = $(DFLAGS) -T $(LSCRIPT) -nostartfiles --specs=nano.specs -specs=nosys.specs -Wl,-Map=$(MAP) -Wl,--gc-sections

# Objecmaket copy (for converting formats)
OBJCOPY = arm-none-eabi-objcopy
OFLAGS = -O ihex

# List generation
OBJDUMP = arm-none-eabi-objdump
DUMPFLAGS = -h -S

CC_SIZE = arm-none-eabi-size

CC_SYMBOLS = arm-none-eabi-nm.exe

RM = rm -rf *.exe *.o *.elf

#-{ Build Rules }---------------------------------------------------------------
################################################################################
# Automatically-generated file. Do not edit!
################################################################################
-include ./Application_Source/subdir.mk
-include ./Device_Drivers/GPIO/subdir.mk
-include ./Libc_Device_Support/subdir.mk
-include ./Special_Libraries/subdir.mk
-include ./Startup/subdir.mk

print_variables:
		@echo 'Object files: $(OBJS)'
		@echo 'Source files: $(C_SRCS)'

# Final binaries
HEX = $(BIN_DIR)/$(APPLICATION_NAME).hex
ELF = $(BIN_DIR)/$(APPLICATION_NAME).elf
LST = $(BIN_DIR)/$(APPLICATION_NAME).lst
SIZ = $(BIN_DIR)/$(APPLICATION_NAME).siz
MAP = $(BIN_DIR)/$(APPLICATION_NAME).map
SYMBOL = $(BIN_DIR)/$(APPLICATION_NAME).text

#-- These rules for the finally binaries will usually not require modification

# Convert the ELF into intel hex format
$(HEX): $(ELF)
	@echo 'Invoking: ARM-GCC Create Flash Image'
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(OBJCOPY) $(OFLAGS) $(ELF) $(HEX)
	@echo 'Finished building: $@'

# Link all intermediate objects into a single executable
$(ELF): $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM-GCC C Linker'
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(LD) $(LFLAGS) $(OBJS) -o $(ELF)
	@echo 'Finished building target: $@'

$(LST): $(ELF)
	@echo 'Invoking: ARM-GCC Create Listing'
	$(OBJDUMP) $(DUMPFLAGS) $^ > $@
	@echo 'Finished building: $@'

$(SIZ): $(ELF)
	@echo 'Invoking: ARM-GCC Print Size'
	$(CC_SIZE) --format=berkeley $^
	@echo 'Finished building: $@'

$(SYMBOL): $(ELF)
	@echo 'Invoking: ARM-GCC Print SYMBOLS'
	$(CC_SYMBOLS) --format=berkeley $^
	@echo 'Finished building: $@'

$(MAP): $(OBJS)
	@echo 'Invoking: ARM-GCC Create MAP'
	$(LD) $(MAPFLAGS) $(OBJS) -o $(MAP)
	@echo 'Finished building: $@'
#-- These rules will vary depending on the program being built

# Phonies
.PHONY: all clean tidy

# Build the entire program
all: $(HEX) $(LST) $(SIZ)

# Delete all of the generated files
clean:
	-del /Q bin\*.hex
	-del /Q bin\*.elf
	-del /Q bin\*.lst
	-del /Q bin\*.map
	@echo Cleaning Completed

# Delete all intermediate object files
tidy:
	-del /Q *.o

# OpenOCD command to program a board
program: $(HEX)
	"$(OPENOCD_DIR)/bin/openocd.exe" -f $(OCD_Interface) -f $(OCD_Target) -c "program $(HEX) verify reset exit"


# OpenOCD command to ERASE a board
erase:
	$(OCD)/bin/openocd.exe -f $(OCD_Interface) -f $(OCD_Target) -c 'kitprog3 power_config on 3300; init; reset init; psoc4 mass_erase 0; shutdown; kitprog3 power_config off; exit'

debug:
	$(OCD)/bin/openocd.exe -s $(OCD_Scripts) -c 'set PSOC4_USE_ACQUIRE 1' -c 'source $(OCD_Interface)' /
	-c 'puts stderr {Started by GNU MCU Eclipse}' -c 'source $(OCD_Target)' /
	-c 'psoc4.cpu configure -rtos auto -rtos-wipe-on-reset-halt 1' /
	-c 'init; reset init'

# Use the variable in a target
custom:
	@echo "Building project in: $(OCD_Scripts)"

# openocd -f 'C:/Infineon/Tools/ModusToolbox/tools_3.0/openocd/scripts/target/psoc4.cfg'

#-{ Resources }-----------------------------------------------------------------

# [1]: https://gcc.gnu.org/onlinedocs/gcc-2.95.3/gcc_2.html#SEC22