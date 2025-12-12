# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
set(PICO_SDK_PATH "C:/VSARM/sdk/pico/pico-sdk")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)


# Set project name and languages
project(phprobe C CXX ASM)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Collect all source files (allows for multiple .c or .cpp files)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Tell CMAKE where to find the executable source file
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

# Other custom libs in src can found by IntelliSense and the compiler
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_adc
)

# Enable USB output, disable UART output
# This allows the Pico to print to a serial terminal over USB
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
