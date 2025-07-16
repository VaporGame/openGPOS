ENTRY(_start)

MEMORY
{
    APP_SRAM (rxw) : ORIGIN = 0x20008000, LENGTH = 0x4000 /* 16KB, leaving 32KB for the kernel (SHRINK LATER) */
}

SECTIONS
{   
    . = ORIGIN(APP_SRAM);
    .text :
    {
        *(.text*)
    } > APP_SRAM
    
    .rodata :
    {
        *(.rodata*)
        *(.srodata*)
    } > APP_SRAM

    .data :
    {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > APP_SRAM

    .bss :
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > APP_SRAM

    .heap :
    {
        __heap_start__ = .;
    } > APP_SRAM

    .stack (NOLOAD):
    {
        . = ORIGIN(APP_SRAM) + LENGTH(APP_SRAM) - 8K;
        _stack_bottom = .;
        . += 8K;
        _stack_top = .;
    } > APP_SRAM

    __heap_end__ = _stack_bottom - 1;
    
    /DISCARD/ :
    {
        *(.comment)
        *(.ARM.attributes)
        *(.debug*)
        *(.note.GNU-stack)
    }
}