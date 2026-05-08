# Pico SDK build for the ATOMiK input controller.
#
# Prereqs:
#   1. Install pico-sdk:  https://github.com/raspberrypi/pico-sdk
#      Set $PICO_SDK_PATH to its checkout.
#   2. Clone Pico-PIO-USB into pico-sdk's lib/ or alongside this project.
#      Set PICO_PIO_USB_PATH if it's not auto-detected.
#
# Build:
#   mkdir build && cd build
#   cmake -DPICO_SDK_PATH=$PICO_SDK_PATH ..
#   make -j
#   # Resulting hid_bridge.uf2 — drag onto the Pico in BOOTSEL mode.

cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK before project() so it can set the toolchain.
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Optional: use the Raspberry Pi Pico W or 2 by uncommenting one:
# set(PICO_BOARD pico_w)
# set(PICO_BOARD pico2)
set(PICO_BOARD pico)

project(hid_bridge C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Pico-PIO-USB — the software USB host stack that runs on PIO + core 1.
# https://github.com/sekigon-gonnoc/Pico-PIO-USB
if(NOT DEFINED PICO_PIO_USB_PATH)
    set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/Pico-PIO-USB)
endif()

add_executable(hid_bridge
    main.c
)

target_include_directories(hid_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_PIO_USB_PATH}/src
)

# TinyUSB host config — board glue + PIO-USB host driver.
target_compile_definitions(hid_bridge PRIVATE
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
)

target_link_libraries(hid_bridge PRIVATE
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_dma
    tinyusb_host
    tinyusb_board
    pico_pio_usb
)

# We DON'T want stdio over the same UART we use to talk to the AX7020.
# Disable stdio-uart entirely; debug prints (if added) go via USB-CDC
# on the Pico's own USB device port.
pico_enable_stdio_usb(hid_bridge 1)
pico_enable_stdio_uart(hid_bridge 0)

# Generate hid_bridge.uf2 + .bin + .hex
pico_add_extra_outputs(hid_bridge)
