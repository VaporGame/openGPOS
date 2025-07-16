ENTRY(bootStage2);

MEMORY
{
    flash(rx) : ORIGIN = 0x10000000, LENGTH = 2048k
    sram(rwx) : ORIGIN = 0x20000000, LENGTH = 256k /* can be adjusted to 264 */
}

SECTIONS
{
    .boot2 :
    {
        _sboot2 = .;
        KEEP(*(.boot2*))
        _eboot2 = .;
        . = . + (252 - (_eboot2 - _sboot2));
        KEEP(*(.crc*))
    } > flash
    
    .text :
    {
        KEEP(*(.vector*))
        *(.text*)
    } > flash
    
    .rodata :
    {
        *(.rodata*)
        *(.srodata*)
    } > flash

    .data :
    {
        __data_start__ = .; 
        *(.data*)
        __data_end__ = .;
    } > sram AT > flash     /* "> sram" is the VMA, "> flash" is the LMA */

    .bss (NOLOAD) :
    {
        __bss_start__ = .;
        *(.bss*)
        __bss_end__ = .;
    } > sram

    .stack (NOLOAD) :
    {
        . = ORIGIN(sram) + LENGTH(sram);
        __stack = .;
    } > sram

    /* Get VMA for .data section */
    _sdataf = LOADADDR(.data);          /* Get starting VMA */

    __heap_start__ = __bss_end__;
    __heap_end__ = __heap_start__ + 0x8000;

    /DISCARD/ :
    {
        *(.comment)
        *(.ARM.attributes)
        *(.debug*)
        *(.note.GNU-stack)
    }
}