################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Special_Libraries/dsplib.c \
../Special_Libraries/reset.c \
../Special_Libraries/cmsis_gcc.c

OBJS += \
./Special_Libraries/dsplib.o \
./Special_Libraries/reset.o \
./Special_Libraries/cmsis_gcc.o

# Each subdirectory must supply rules for building sources it contributes
Special_Libraries/%.o: ../Special_Libraries/%.c
	@echo 'Building file: $^'
	@echo 'Invoking: ARM-GCC C Compiler'
	$(CC) $(CFLAGS) -c -o $@ $^
	@echo 'Finished building: $^'

