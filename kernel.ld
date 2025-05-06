OUTPUT_ARCH( "riscv" )

ENTRY( _start )

/* The 'virt' virtual machine is super simple. No ROM relocations or pain. */
MEMORY
{
  RAM   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
  __stack_size = DEFINED(__stack_size) ? __stack_size : 4K;
  __heap_size = DEFINED(__heap_size) ? __heap_size : 32M;

  /* Group all the immutable (code, unchanged data) together */
  .text :
  {
    PROVIDE(__text_start = .);
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
    PROVIDE(__text_end = .);
  } >RAM

  . = ALIGN(4);

  .data :
  {
    PROVIDE(_sdata = .);
    PROVIDE(_gp = . + 0x800);
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_edata = .);
  }

  /* Shove all the zero-initialized sections together. */
  .bss :
  {
    PROVIDE(_sbss = .);
    *(.sbss*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    PROVIDE(_ebss = .);
  } > RAM

  . = ALIGN(8);
  PROVIDE( _end = . );

  /* Куча (heap) — располагается после BSS */
  .heap : {
      . = ALIGN(4096);
      PROVIDE(__heap_start = .);
      . = . + __heap_size;  /* Резервируем __heap_size байт */
      PROVIDE(__heap_end = .);
  } >RAM

  /* Стек — в конце RAM, растёт вниз */
  .stack : {
      . = ALIGN(16);  /* Выравнивание стека */
      PROVIDE(_sp = . + __stack_size);  /* Указываем вершину стека */
  } >RAM
}
