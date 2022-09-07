SECTIONS
{
    . = 0xFFFF000000080000;
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
    PROVIDE(etext = .);
    .init : { 
        PROVIDE(early_init = .);
        *(.init.early)
        PROVIDE(rest_init = .);
        *(.init.rest)
        PROVIDE(init = .);
        *(.init)
        PROVIDE(einit = .);
    }
    .rodata : { *(.rodata) }
    PROVIDE(data = .);
    .data : { *(.data) }
    PROVIDE(edata = .);
    .bss : { *(.bss) }
    PROVIDE(end = .);
}
