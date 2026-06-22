OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);  /* R + X */
  data PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS
{
  . = 0x0;

  /* --- text section (コード) --- */
  _text = .;
  .text : {
    *(.text)
  } :text
  _etext = .;

  /* --- 読み取り専用データ --- */
  .rodata : {
    *(.rodata*)
  } :text

  /* --- 書き込み可能データ --- */
  .data : {
    *(.data)
  } :data
  _edata = .;

  /* --- BSS領域 (ゼロ初期化データ) --- */
  .bss (NOLOAD) : {
    *(.bss)
    *(COMMON)
  } :data
  _end = .;

  /* --- スタック領域 (必要に応じて定義) --- */
  . = ALIGN(16);
  stack_start = .;
  . += 0x10000; /* 64KB スタック確保 */
  stack_end = .;

  /DISCARD/ : {
    *(.note.GNU-stack)
  }
}
