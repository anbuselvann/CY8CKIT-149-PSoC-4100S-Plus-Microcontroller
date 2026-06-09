################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libc_Device_Support/malloc_support.c \
../Libc_Device_Support/printf_support.c \
../Libc_Device_Support/syscalls.c

OBJS += \
./Libc_Device_Support/malloc_support.o \
./Libc_Device_Support/printf_support.o \
./Libc_Device_Support/syscalls.o \

# Each subdirectory must supply rules for building sources it contributes
Libc_Device_Support/%.o: ../Libc_Device_Support/%.c
	@echo 'Building file: $^'
	@echo 'Invoking: ARM-GCC C Compiler'
	$(CC) $(CFLAGS) -c -o $@ $^
	@echo 'Finished building: $^'

