################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Project_Settings/Startup_Code/startup.c 

OBJS += \
./Project_Settings/Startup_Code/startup.o 

C_DEPS += \
./Project_Settings/Startup_Code/startup.d 


# Each subdirectory must supply rules for building sources it contributes
Project_Settings/Startup_Code/%.o: ../Project_Settings/Startup_Code/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"C:\Users\98112939\Documents\PortableGit\home\portable\48434-Embedded-Software-Project\Project\Library" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Static_Code/IO_Map" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Sources" -I"C:/Users/98112939/Documents/PortableGit/home/portable/48434-Embedded-Software-Project/Project/Generated_Code" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


