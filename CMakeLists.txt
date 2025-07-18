cmake_minimum_required(VERSION 3.15)

project(freeGPOS VERSION 0.1)

set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PROJECT_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

set(SRCDIR ${CMAKE_SOURCE_DIR}/src)
set(RP2040DIR ${SRCDIR}/arch/arm_cortexm/rp2040)

set(BOOT2DIR ${RP2040DIR}/boot2)
set(BOOT2 bootStage2QuadIO)
set(COMPCRC compCrc32)
set(CRCVALUE crc)

set(LNKSCRIPT ${SRCDIR}/arch/arm_cortexm/linker.ld)

set(BUILDDIR ${PROJECT_BINARY_DIR})
set(BUILDBOOT2DIR ${BUILDDIR}/boot2)

set(TOOLCHAIN arm-none-eabi-)
set(CMAKE_C_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN}objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN}objdump")

set(UTILS ${CMAKE_SOURCE_DIR}/utils)

set(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -O3 --specs=nano.specs -ffreestanding -nostdlib -ffunction-sections -fdata-sections")

set(SOURCES
    ${SRCDIR}/kernel/main.c
    ${SRCDIR}/kernel/syscall.c
    ${RP2040DIR}/system_rp2040.c
    ${RP2040DIR}/startup_rp2040.c
)

file(GLOB_RECURSE LIB
    "${SRCDIR}/lib/*.c"
)
# list(APPEND SOURCES ${LIB})

add_library(lib STATIC ${LIB})
target_include_directories(lib PUBLIC ${SRCDIR}/lib)

file(GLOB_RECURSE LIB_C
    "${CMAKE_SOURCE_DIR}/libc/src/*.c"
)
#list(APPEND SOURCES ${LIB_C})

add_library(libc STATIC ${LIB_C})
target_include_directories(libc PUBLIC ${CMAKE_SOURCE_DIR}/libc/include)

file(GLOB_RECURSE DRIVER
    "${SRCDIR}/driver/*.c"
)
list(APPEND SOURCES ${DRIVER})

include_directories(
    ${SRCDIR}/lib
    ${SRCDIR}/driver
    ${SRCDIR}/arch/arm_cortexm/rp2040/
    ${CMAKE_SOURCE_DIR}/libc/include
    ${SRCDIR}/kernel
)

# Compile bootStage2 with linking
add_executable(${BOOT2}.elf ${BOOT2DIR}/${BOOT2}.c)
target_link_options(${BOOT2}.elf PRIVATE -T ${LNKSCRIPT} -Wl,--gc-sections)
set_target_properties(${BOOT2}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BUILDBOOT2DIR}/"
)

# First, compile compCrc32.cpp to create the executable
add_executable(compCrc32.out ${BOOT2DIR}/compCrc32.cpp)
target_compile_options(compCrc32.out PUBLIC -I ${UTILS})
set_target_properties(compCrc32.out PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BUILDBOOT2DIR}/"
)

# Compute CRC32 value and generate a c code for it
add_custom_command(
    OUTPUT ${BUILDBOOT2DIR}/${CRCVALUE}.c
    COMMAND ${CMAKE_OBJCOPY} -O binary ${BUILDBOOT2DIR}/${BOOT2}.elf ${BUILDBOOT2DIR}/${BOOT2}.bin
    COMMAND ${BUILDBOOT2DIR}/${COMPCRC}.out ${BUILDBOOT2DIR}/${BOOT2}.bin
    DEPENDS ${BUILDBOOT2DIR}/${BOOT2}.elf
)

# Compile the project and link everything into an elf file
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${BOOT2DIR}/${BOOT2}.c ${BUILDBOOT2DIR}/${CRCVALUE}.c)
target_link_options(${PROJECT_NAME}.elf PRIVATE -T ${LNKSCRIPT} -Wl,--gc-sections,-Map=${CMAKE_BINARY_DIR}/map.map)
target_link_libraries(${PROJECT_NAME}.elf PUBLIC gcc libc lib)

# Convert elf to bin to uf2 file
add_custom_target(${PROJECT_NAME}.uf2 ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary ${BUILDDIR}/${PROJECT_NAME}.elf ${BUILDDIR}/${PROJECT_NAME}.bin
    COMMAND python3 ${UTILS}/uf2/utils/uf2conv.py -b 0x10000000 -f 0xe48bff56 -c ${BUILDDIR}/${PROJECT_NAME}.bin -o ${BUILDDIR}/${PROJECT_NAME}.uf2
    DEPENDS ${PROJECT_NAME}.elf
)

# Automatically flash to pico
add_custom_target(flash ALL
    COMMAND picotool load ${BUILDDIR}/${PROJECT_NAME}.uf2
    COMMAND picotool reboot
    DEPENDS ${BUILDDIR}/${PROJECT_NAME}.uf2
)