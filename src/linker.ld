SECTIONS
{
    . = 0xFFFF000000080000;
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
    PROVIDE(etext = .);
    . = ALIGN(8);
    .init : { 
        PROVIDE(early_init = .);
        KEEP(*(.init.early))
        PROVIDE(rest_init = .);
        KEEP(*(.init.rest))
        PROVIDE(init = .);
        KEEP(*(.init))
        PROVIDE(einit = .);
    }
    .rodata : { *(.rodata) }
    PROVIDE(data = .);
    .data : { *(.data) }
    PROVIDE(edata = .);
    .bss : { *(.bss) }
    PROVIDE(end = .);
}
