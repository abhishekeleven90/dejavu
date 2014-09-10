################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main_a.cpp \
../src/main_h.cpp \
../src/temp_h.cpp 

OBJS += \
./src/main_a.o \
./src/main_h.o \
./src/temp_h.o 

CPP_DEPS += \
./src/main_a.d \
./src/main_h.d \
./src/temp_h.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


