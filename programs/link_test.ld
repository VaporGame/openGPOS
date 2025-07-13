ENTRY(main)

MEMORY
{
    APP_SRAM (rxw) : ORIGIN = 0x20008000, LENGTH = 0x39000 /* 232KB, leaving 32KB for the kernel (SHRINK LATER) */
}

SECTIONS
{
    .text :
    {
        *(.text*)
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

    _end = .;

    __stack = ORIGIN(APP_SRAM) + LENGTH(APP_SRAM);
}