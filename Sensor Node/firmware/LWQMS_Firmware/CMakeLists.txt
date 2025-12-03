# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(LWQMS_Firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
file(GLOB DRIVER_SOURCES "sx126x_driver/*.c")
file(GLOB LIB "lib/*.c")
file(GLOB LIB_EXTERNAL "lib/external/*.c")
file(GLOB SYS_UTILS "sys_utils/*.c")

add_executable(LWQMS_Firmware 
    main.c
    ${DRIVER_SOURCES}
    ${LIB}
    ${LIB_EXTERNAL}
    ${SYS_UTILS}
)

pico_set_program_name(LWQMS_Firmware "LWQMS_Firmware")
pico_set_program_version(LWQMS_Firmware "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(LWQMS_Firmware 0)
pico_enable_stdio_usb(LWQMS_Firmware 1)

# Add the standard library to the build
target_link_libraries(
    LWQMS_Firmware
    pico_stdlib
    hardware_spi
    hardware_watchdog
    hardware_irq
    hardware_resets
    hardware_i2c
    hardware_powman
)

# Include these directories in the source search
target_include_directories(LWQMS_Firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/sys_utils
        ${CMAKE_CURRENT_LIST_DIR}/sys_utils/headers
        ${CMAKE_CURRENT_LIST_DIR}/sx126x_driver
        ${CMAKE_CURRENT_LIST_DIR}/sx126x_driver/headers
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/lib/headers
        ${CMAKE_CURRENT_LIST_DIR}/lib/external
)

# Enable creation of .uf2 file.
pico_add_extra_outputs(LWQMS_Firmware)

# Create a top-level directory named "bin" to store the final build binary
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add a custom command that runs after the build is complete.
# This copies the .uf2 file from the build directory to the "bin" directory.
add_custom_command(
    TARGET LWQMS_Firmware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/LWQMS_Firmware.uf2
            ${CMAKE_SOURCE_DIR}/bin/LWQMS_Firmware.uf2
    COMMENT "Copying .uf2 to bin/ directory"
)