cmake_minimum_required(VERSION 3.12)

# Comment the line below in for the Pico W examples to build
# set(PICO_BOARD pico_w)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_SDK_PATH "/Users/vayuntiwari/.pico-sdk/sdk/2.1.1")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(Resistive_Touchscreen C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(HUNTER_PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

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
set(PICO_BOARD pico CACHE STRING "Board type")

add_subdirectory(
  ${PROJECT_SOURCE_DIR}/third_party/pico-tflmicro  
  pico-tflmicro-build
)

add_executable(Resistive_Touchscreen)

# must match with executable name and source file names
target_sources(Resistive_Touchscreen PRIVATE trackpad.c classify.cpp vga16_graphics.c third_party/OLED_SSH1106_I2C/ssd1306.c
third_party/OLED_SSH1106_I2C/font.c
)

target_include_directories(Resistive_Touchscreen PRIVATE ${PROJECT_SOURCE_DIR}/third_party/pico-tflmicro/src third_party/OLED_SSH1106_I2C)


# must match with pio filename and executable name from above
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)




# must match with executable name
target_link_libraries(Resistive_Touchscreen PRIVATE pico_stdlib hardware_pio hardware_dma hardware_adc hardware_irq hardware_adc pico-tflmicro hardware_i2c)

# must match with executable name
pico_add_extra_outputs(Resistive_Touchscreen)
