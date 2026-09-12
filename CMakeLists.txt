# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

project(skylander_portal_emu C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()



add_executable(skylander_portal_emu
        main.c
        )
#target_compile_definitions(skylander_portal_emu PRIVATE MBEDTLS_CONFIG_FILE="mbedtls_config.h")
target_sources(skylander_portal_emu PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/rijndael.c
        ${CMAKE_CURRENT_LIST_DIR}/SkylanderCrypt.c
        ${CMAKE_CURRENT_LIST_DIR}/hw_config.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )
        
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
target_include_directories(skylander_portal_emu PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

# pull in common dependencies 
target_link_libraries(skylander_portal_emu PUBLIC pico_stdlib tinyusb_device tinyusb_board hardware_i2c hardware_gpio FatFs_SPI pico_mbedtls)
pico_enable_stdio_uart(skylander_portal_emu 1)
# Tell CMake where to find other source code

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(skylander_portal_emu)


