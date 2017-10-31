################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/Constants.c \
../Sources/FIFO.c \
../Sources/FTM.c \
../Sources/FixedPoint.c \
../Sources/Flash.c \
../Sources/HMI.c \
../Sources/LED.c \
../Sources/LPT.c \
../Sources/Measurements.c \
../Sources/PIT.c \
../Sources/RTC.c \
../Sources/SelfTest.c \
../Sources/TowerProtocol.c \
../Sources/UART.c \
../Sources/main.c \
../Sources/packet.c 

OBJS += \
./Sources/Constants.o \
./Sources/FIFO.o \
./Sources/FTM.o \
./Sources/FixedPoint.o \
./Sources/Flash.o \
./Sources/HMI.o \
./Sources/LED.o \
./Sources/LPT.o \
./Sources/Measurements.o \
./Sources/PIT.o \
./Sources/RTC.o \
./Sources/SelfTest.o \
./Sources/TowerProtocol.o \
./Sources/UART.o \
./Sources/main.o \
./Sources/packet.o 

C_DEPS += \
./Sources/Constants.d \
./Sources/FIFO.d \
./Sources/FTM.d \
./Sources/FixedPoint.d \
./Sources/Flash.d \
./Sources/HMI.d \
./Sources/LED.d \
./Sources/LPT.d \
./Sources/Measurements.d \
./Sources/PIT.d \
./Sources/RTC.d \
./Sources/SelfTest.d \
./Sources/TowerProtocol.d \
./Sources/UART.d \
./Sources/main.d \
./Sources/packet.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"C:\Users\98112939\Documents\PortableGit\home\portable\48434-Embedded-Software-Project\Project\Library" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Static_Code/IO_Map" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Sources" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Generated_Code" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


