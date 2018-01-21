################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/adc.c \
../Src/adc_external_ADS8638.c \
../Src/ain.c \
../Src/can.c \
../Src/canbus.c \
../Src/canopen.c \
../Src/dac.c \
../Src/dac_out.c \
../Src/digital_in_out.c \
../Src/dma.c \
../Src/fatfs.c \
../Src/freertos.c \
../Src/gpio.c \
../Src/iwdg.c \
../Src/lens_control.c \
../Src/main.c \
../Src/nmt.c \
../Src/nmtmaster.c \
../Src/objectdictionary.c \
../Src/quadspi.c \
../Src/sdo.c \
../Src/sdoclient.c \
../Src/spi.c \
../Src/stm32f7xx_hal_msp.c \
../Src/stm32f7xx_hal_timebase_TIM.c \
../Src/stm32f7xx_it.c \
../Src/system_stm32f7xx.c \
../Src/tim.c \
../Src/user_diskio.c 

OBJS += \
./Src/adc.o \
./Src/adc_external_ADS8638.o \
./Src/ain.o \
./Src/can.o \
./Src/canbus.o \
./Src/canopen.o \
./Src/dac.o \
./Src/dac_out.o \
./Src/digital_in_out.o \
./Src/dma.o \
./Src/fatfs.o \
./Src/freertos.o \
./Src/gpio.o \
./Src/iwdg.o \
./Src/lens_control.o \
./Src/main.o \
./Src/nmt.o \
./Src/nmtmaster.o \
./Src/objectdictionary.o \
./Src/quadspi.o \
./Src/sdo.o \
./Src/sdoclient.o \
./Src/spi.o \
./Src/stm32f7xx_hal_msp.o \
./Src/stm32f7xx_hal_timebase_TIM.o \
./Src/stm32f7xx_it.o \
./Src/system_stm32f7xx.o \
./Src/tim.o \
./Src/user_diskio.o 

C_DEPS += \
./Src/adc.d \
./Src/adc_external_ADS8638.d \
./Src/ain.d \
./Src/can.d \
./Src/canbus.d \
./Src/canopen.d \
./Src/dac.d \
./Src/dac_out.d \
./Src/digital_in_out.d \
./Src/dma.d \
./Src/fatfs.d \
./Src/freertos.d \
./Src/gpio.d \
./Src/iwdg.d \
./Src/lens_control.d \
./Src/main.d \
./Src/nmt.d \
./Src/nmtmaster.d \
./Src/objectdictionary.d \
./Src/quadspi.d \
./Src/sdo.d \
./Src/sdoclient.d \
./Src/spi.d \
./Src/stm32f7xx_hal_msp.d \
./Src/stm32f7xx_hal_timebase_TIM.d \
./Src/stm32f7xx_it.d \
./Src/system_stm32f7xx.d \
./Src/tim.d \
./Src/user_diskio.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F767xx -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Inc" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Drivers/STM32F7xx_HAL_Driver/Inc" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Drivers/STM32F7xx_HAL_Driver/Inc/Legacy" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Drivers/CMSIS/Device/ST/STM32F7xx/Include" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Middlewares/Third_Party/FatFs/src" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Middlewares/Third_Party/FreeRTOS/Source/include" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" -I"/Users/rossco/Documents/workspace/STM32F7/V2_lens_monitor/Lens_Monitor_Thomas_Original/Lens_Monitor_Thomas/Drivers/CMSIS/Include"  -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


