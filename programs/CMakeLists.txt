cmake_minimum_required(VERSION 3.15)

project(testprogram VERSION 0.1)

set(SRCDIR ${CMAKE_SOURCE_DIR}/../src)

set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PROJECT_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

set(LNKSCRIPT ${CMAKE_SOURCE_DIR}/link_test.ld)

set(BUILDDIR ${PROJECT_BINARY_DIR})

set(TOOLCHAIN arm-none-eabi-)
set(CMAKE_C_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN}objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN}objdump")

set(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -Os --specs=nano.specs -ffreestanding -ffunction-sections -fdata-sections")

set(SOURCES
    ${CMAKE_SOURCE_DIR}/init.c
    ${SRCDIR}/arch/arm_cortexm/rp2040/crt0.c
)

file(GLOB_RECURSE LIB
    "${SRCDIR}/lib/*.c"
)

add_library(lib STATIC ${LIB})
target_include_directories(lib PUBLIC ${SRCDIR}/lib)

file(GLOB_RECURSE LIB_C
    "${CMAKE_SOURCE_DIR}/../libc/src/*.c"
)

add_library(libc STATIC ${LIB_C})
target_include_directories(libc PUBLIC ${CMAKE_SOURCE_DIR}/../libc/include)

file(GLOB_RECURSE DRIVER
    "${SRCDIR}/../driver/*.c"
)
list(APPEND SOURCES ${DRIVER})

include_directories(
    ${SRCDIR}/lib
    ${SRCDIR}/driver
    ${SRCDIR}/arch/arm_cortexm/rp2040/
    ${CMAKE_SOURCE_DIR}/../libc/include
    ${SRCDIR}/kernel
)

add_executable(init.elf ${SOURCES})
target_link_options(init.elf PRIVATE -nostdlib -nostartfiles -T ${LNKSCRIPT} -Wl,--gc-sections)
target_link_libraries(init.elf PRIVATE gcc libc lib)
set_target_properties(init.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out"
)