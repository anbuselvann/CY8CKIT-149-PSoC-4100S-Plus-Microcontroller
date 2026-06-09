################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Startup/startup.c

OBJS += \
./Startup/startup.o

# Each subdirectory must supply rules for building sources it contributes
Startup/%.o: ../Startup/%.c
	@echo 'Building file: $^'
	@echo 'Invoking: ARM-GCC C Compiler'
	$(CC) $(CFLAGS) -c -o $@ $^
	@echo 'Finished building: $^'

