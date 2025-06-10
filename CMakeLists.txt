cmake_minimum_required(VERSION 3.31)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
set (PICO_PLATFORM rp2350 CACHE STRING "Platform type")

#Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set (PROJECT hello_serial)
project(hello_serial C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(hello_serial
        hello_serial.c src/gps.c src/uart.c
        )

# pull in common dependencies
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT} pico_stdlib hardware_uart)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${PROJECT})

# open serial port
#pico_enable_stdio_uart(${PROJECT} 1)
pico_enable_stdio_usb(${PROJECT} 1)