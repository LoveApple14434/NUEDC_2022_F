# ARM Compiler (Keil) 工具链配置
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 设置编译器路径（根据实际Keil安装路径修改）
set(KEIL_PATH "D:/Program/Keil_v5/")
set(ARMCC_BIN "${KEIL_PATH}ARM/ARM_Compiler_5.06u7/bin/")
set(ARMCLANG_BIN "${KEIL_PATH}ARM/ARMCLANG/bin/")

# 根据项目配置选择编译器
if ("armcc" STREQUAL "armclang")
    set(CMAKE_C_COMPILER "${ARMCLANG_BIN}armclang.exe")
    set(CMAKE_CXX_COMPILER "${ARMCLANG_BIN}armclang.exe")
    set(CMAKE_ASM_COMPILER "${ARMCLANG_BIN}armclang.exe")
    set(CMAKE_LINKER "${ARMCLANG_BIN}armlink.exe")
    set(CMAKE_AR "${ARMCLANG_BIN}armar.exe")
    set(CMAKE_FROMELF "${ARMCLANG_BIN}fromelf.exe")
else()
    set(CMAKE_C_COMPILER "${ARMCC_BIN}armcc.exe")
    set(CMAKE_CXX_COMPILER "${ARMCC_BIN}armcc.exe")
    set(CMAKE_ASM_COMPILER "${ARMCC_BIN}armasm.exe")
    set(CMAKE_LINKER "${ARMCC_BIN}armlink.exe")
    set(CMAKE_AR "${ARMCC_BIN}armar.exe")
    set(CMAKE_FROMELF "${ARMCC_BIN}fromelf.exe")
endif()

# 确保使用正确的工具链
set(CMAKE_C_COMPILER_WORKS 1 INTERNAL)
set(CMAKE_ASM_COMPILER_WORKS 1 INTERNAL)
set(CMAKE_CXX_COMPILER_WORKS 1 INTERNAL)

# 公共编译器选项
set(COMMON_FLAGS "--cpu=Cortex-M4 --apcs=interwork --c99 --split_sections ")

# 汇编器选项
set(ASM_FLAGS "--cpu=Cortex-M4 --apcs=interwork --pd \"__MICROLIB SETA 1\" ")

# 链接器选项
set(LINKER_FLAGS
    "--map"
    "--info=summarysizes,sizes,totals,unused,veneers"
    "--callgraph"
    "--xref"
    "--entry=Reset_Handler"
    "--strict"
    "--summary_stderr"
    ""
)

# 配置编译器选项
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -O0 --cpp")
set(CMAKE_ASM_FLAGS "${ASM_FLAGS}")

# 设置链接器
set(CMAKE_C_LINK_EXECUTABLE 
    "${CMAKE_LINKER} <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
)

# 交叉编译设置（跳过编译器测试）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 构建类型配置
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug or Release)")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if ("armcc" STREQUAL "armclang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --debug")
    endif()
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --no_debug")
endif()

# 设置目标属性
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 添加诊断信息
message(STATUS "=== 使用 ARM 工具链 ===")
message(STATUS "编译器类型: ${CMAKE_C_COMPILER}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
