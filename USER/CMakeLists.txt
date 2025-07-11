cmake_minimum_required(VERSION 4.0)

# 包含工具链配置
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake)

project(UCOSII-1-2 LANGUAGES C ASM)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    ./main.c
    ./stm32f4xx_it.c
    ./system_stm32f4xx.c
    ./stm32f4xx_hal_msp.c
    ../CORE/startup_stm32f407xx.s
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_usart.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sram.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fsmc.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac.c
    ../HALLIB/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac_ex.c
    ../SYSTEM/delay/delay.c
    ../SYSTEM/sys/sys.c
    ../SYSTEM/usart/usart.c
    ../UCOSII/UCOS_BSP/bsp.c
    ../UCOSII/UCOS_BSP/cpu_bsp.c
    ../UCOSII/uC-CPU/ARM-Cortex-M4/RealView/cpu_c.c
    ../UCOSII/uC-CPU/cpu_core.c
    ../UCOSII/uC-CPU/ARM-Cortex-M4/RealView/cpu_a.asm
    ../UCOSII/uC-LIB/lib_ascii.c
    ../UCOSII/uC-LIB/lib_math.c
    ../UCOSII/uC-LIB/lib_mem.c
    ../UCOSII/uC-LIB/lib_str.c
    ../UCOSII/uC-LIB/Ports/ARM-Cortex-M4/RealView/lib_mem_a.asm
    ../UCOSII/uCOS-II/Source/os_core.c
    ../UCOSII/uCOS-II/Source/os_flag.c
    ../UCOSII/uCOS-II/Source/os_mbox.c
    ../UCOSII/uCOS-II/Source/os_mem.c
    ../UCOSII/uCOS-II/Source/os_mutex.c
    ../UCOSII/uCOS-II/Source/os_q.c
    ../UCOSII/uCOS-II/Source/os_sem.c
    ../UCOSII/uCOS-II/Source/os_task.c
    ../UCOSII/uCOS-II/Source/os_time.c
    ../UCOSII/uCOS-II/Source/os_tmr.c
    ../UCOSII/uCOS-II/Ports/ARM-Cortex-M4/Generic/RealView/os_cpu_c.c
    ../UCOSII/uCOS-II/Ports/ARM-Cortex-M4/Generic/RealView/os_cpu_a.asm
    ../UCOSII/uCOS-CONFIG/app_hooks.c
    ../HARDWARE/SRC/AD9959.c
    ../SYSTEM/usart/usart.c
    ../HARDWARE/SRC/led.c
    ../HARDWARE/SRC/lcd.c
    ../HARDWARE/SRC/key.c
    ../HARDWARE/SRC/24cxx.c
    ../HARDWARE/SRC/myiic.c
    ../HARDWARE/SRC/ctiic.c
    ../HARDWARE/SRC/ft5206.c
    ../HARDWARE/SRC/gt9147.c
    ../HARDWARE/SRC/ott2001a.c
    ../HARDWARE/SRC/touch.c
    ../HARDWARE/SRC/AD9959.c
    ../HARDWARE/SRC/adc.c
    ../HARDWARE/SRC/timer.c
    ../HARDWARE/SRC/switch.c
    ../USER/rfft.c
)

# 添加自定义目标用于生成二进制文件 (使用 POST_BUILD)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_FROMELF} --i32combined --output="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${PROJECT_NAME}.hex" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Generating HEX file"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_FROMELF} --bin --output="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${PROJECT_NAME}.bin" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Generating BIN file"
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ../CORE
    ../OBJ
    ../USER
    ../SYSTEM/delay
    ../SYSTEM/sys
    ../SYSTEM/usart
    ../HALLIB/STM32F4xx_HAL_Driver/Inc
    ../UCOSII/uC-LIB
    ../UCOSII/uC-CPU
    ../UCOSII/UCOS_BSP
    ../UCOSII/uCOS-CONFIG
    ../UCOSII/uCOS-II/Source
    ../UCOSII/uC-CPU/ARM-Cortex-M4/RealView
    ../UCOSII/uC-LIB/Ports/ARM-Cortex-M4/RealView
    ../UCOSII/uCOS-II/Ports/ARM-Cortex-M4/Generic/RealView
    ../HARDWARE/INC
)

# 预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F407xx
)

# 链接器脚本设置
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/../OBJ/Template.sct")

# 链接选项
target_link_options(${PROJECT_NAME} PRIVATE
    ${LINKER_FLAGS}
    "--scatter=${LINKER_SCRIPT}"
    
)
