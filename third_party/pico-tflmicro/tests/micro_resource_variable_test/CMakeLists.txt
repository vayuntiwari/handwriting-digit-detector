cmake_minimum_required(VERSION 3.12)

project(micro_resource_variable_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(micro_resource_variable_test "")

target_include_directories(micro_resource_variable_test
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../tests/micro_resource_variable_test
)

set_target_properties(
  micro_resource_variable_test
  PROPERTIES
  COMPILE_FLAGS -Os
  COMPILE_FLAGS -fno-rtti
  COMPILE_FLAGS -fno-exceptions
  COMPILE_FLAGS -fno-threadsafe-statics
  COMPILE_FLAGS -nostdlib
)

pico_enable_stdio_usb(micro_resource_variable_test 1)
pico_enable_stdio_uart(micro_resource_variable_test 0)

target_sources(micro_resource_variable_test
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../tests/micro_resource_variable_test/micro_resource_variable_test.cpp

)

target_link_libraries(
  micro_resource_variable_test
  pico-tflmicro
  pico-tflmicro_test
)

pico_add_extra_outputs(micro_resource_variable_test)
