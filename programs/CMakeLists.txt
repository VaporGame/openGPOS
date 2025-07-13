cmake_minimum_required(VERSION 3.15)

project(testprogram VERSION 0.1)

set(SRCDIR ../src)

set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PROJECT_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

set(LNKSCRIPT ${CMAKE_SOURCE_DIR}/link_test.ld)

set(BUILDDIR ${PROJECT_BINARY_DIR})

set(TOOLCHAIN arm-none-eabi-)
set(CMAKE_C_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN}objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN}objdump")

set(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -Os --specs=nano.specs -ffreestanding -nostdlib --static ")

set(SOURCES
    ${CMAKE_SOURCE_DIR}/test.c
    ${SRCDIR}/hardware/uart.c
    ${SRCDIR}/hexutils.c
)

file(GLOB_RECURSE LIB_C_FILES
    "${CMAKE_SOURCE_DIR}/libc/src/*.c"
)

list(APPEND SOURCES ${LIB_C_FILES})

include_directories("${CMAKE_SOURCE_DIR}/../include/")

add_executable(testProg.elf ${SOURCES})
target_link_options(testProg.elf PUBLIC -T ${LNKSCRIPT})
target_include_directories(testProg.elf PUBLIC ${CMAKE_SOURCE_DIR}/../libc/include)
target_link_libraries(testProg.elf PUBLIC gcc)
set_target_properties(testProg.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out"
)