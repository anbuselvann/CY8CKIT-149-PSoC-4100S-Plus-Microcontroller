################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Device_Drivers/GPIO/gpio.c

OBJS += \
./Device_Drivers/GPIO/gpio.o

# Each subdirectory must supply rules for building sources it contributes
Device_Drivers/GPIO/%.o: ../Device_Drivers/GPIO/%.c
	@echo 'Building file: $^'
	@echo 'Invoking: ARM-GCC C Compiler'
	$(CC) $(CFLAGS) -c -o $@ $^ 
	@echo 'Finished building: $^'

