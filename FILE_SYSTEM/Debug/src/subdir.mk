################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/file_system_config.c \
../src/file_system_main.c \
../src/file_system_thread.c \
../src/file_system_utils.c 

C_DEPS += \
./src/file_system_config.d \
./src/file_system_main.d \
./src/file_system_thread.d \
./src/file_system_utils.d 

OBJS += \
./src/file_system_config.o \
./src/file_system_main.o \
./src/file_system_thread.o \
./src/file_system_utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/file_system_config.d ./src/file_system_config.o ./src/file_system_main.d ./src/file_system_main.o ./src/file_system_thread.d ./src/file_system_thread.o ./src/file_system_utils.d ./src/file_system_utils.o

.PHONY: clean-src

