MEMORY
{
    BROM : ORIGIN = 0x30000000 , LENGTH = 256K
    SRAM_S0 : ORIGIN = 0x30040000 + 0x0 , LENGTH = 0xC000 - 0xc00 - 0x0
    SRAM_S1_SW : ORIGIN = 0x40000000 - 0 , LENGTH = 0x0
    SRAM_S1_CMA : ORIGIN = 0x40000000 - 0 + 0x0 , LENGTH = 0 - 0x0
    FLASH_XIP : ORIGIN = 0x60000000 + 0x80800 , LENGTH = 512M - 0x80800
}
PROVIDE (__sram_s0_start = 0x30040000);
PROVIDE (__sram_s0_end = __sram_s0_start + 0xC000 - 0);
PROVIDE (__sram_s1_sw_start = 0x40000000 - 0);
PROVIDE (__sram_s1_sw_end = 0x40000000 - 0 + 0x0);
PROVIDE (__sram_s1_cma_start = __sram_s1_sw_end);
PROVIDE (__sram_s1_cma_end = 0x40000000);
PROVIDE (__sram_s0_sw_heap_end = __sram_s0_end);
PROVIDE (__sram_s1_cma_heap_end = __sram_s1_cma_end);
PROVIDE (__sram_s1_sw_heap_end = __sram_s1_sw_end);
PROVIDE (__min_heap_size = 0x200);
PROVIDE (__sram_sw_heap_start = __sram_s0_sw_heap_start);
PROVIDE (__sram_sw_heap_end = __sram_s0_sw_heap_end);
PROVIDE (__heap_start = __sram_s0_sw_heap_start);
PROVIDE (__heap_end = __sram_s0_sw_heap_end);
PROVIDE (__cma_heap_start = __sram_s1_cma_heap_start);
PROVIDE (__cma_heap_end = __sram_s1_cma_heap_end);
REGION_ALIAS("REGION_SRAM_S0_SW" , SRAM_S0);
REGION_ALIAS("REGION_SRAM_S1_CMA" , SRAM_S1_CMA);
REGION_ALIAS("REGION_SRAM_S1_SW" , SRAM_S1_SW);
REGION_ALIAS("REGION_TEXT" , FLASH_XIP);
REGION_ALIAS("REGION_RODATA" , FLASH_XIP);
REGION_ALIAS("REGION_DATA" , SRAM_S0);
REGION_ALIAS("REGION_BSS" , SRAM_S0);
ENTRY(Reset_Handler)
SECTIONS
{
 .text : AT(ADDR(.text)){
  . = ALIGN(0x4) ;
  __stext = . ;
  KEEP(*startup_gcc.o(*.text*))
  *(.text)
  *(.text*)
  *(.text.*)
  *(.gnu.warning)
  *(.stub)
  *(.gnu.linkonce.t*)
  *(.glue_7t)
  *(.glue_7)
  *(.jcr)
  *(.init)
  *(.fini)
  . = ALIGN (4) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)
  . = ALIGN(4) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(4) ;
  __console_init_end = .;
  . = ALIGN(4);
  __fsymtab_start = .;
  KEEP(*(FSymTab))
  __fsymtab_end = .;
  . = ALIGN(4);
  __vsymtab_start = .;
  KEEP(*(VSymTab))
  __vsymtab_end = .;
  . = ALIGN(4);
  . = ALIGN(4);
  __rt_init_start = .;
  KEEP(*(SORT(.rti_fn*)))
  __rt_init_end = .;
  . = ALIGN(4);
  . = ALIGN(0x10) ;
  __etext = . ;
 } > REGION_TEXT
 .eh_frame_hdr : {
  *(.eh_frame_hdr)
 } > REGION_TEXT
 .eh_frame : ONLY_IF_RO {
  KEEP (*(.eh_frame))
 } > REGION_TEXT
 .gcc_except_table : ONLY_IF_RO {
  *(.gcc_except_table .gcc_except_table.*)
 } > REGION_TEXT
 .rodata :{
  . = ALIGN(0x4) ;
  __srodata = .;
  *(.rdata)
  *(.rdata*)
  *(.rdata1)
  *(.rdata.*)
  *(.rodata)
  *(.rodata1)
  *(.rodata*)
  *(.rodata.*)
  *(.srodata*)
  *(.rodata.str1.4)
  . = ALIGN(0x4) ;
    PROVIDE(__ctors_start__ = .);
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array))
  PROVIDE(__ctors_end__ = .);
  PROVIDE(__dtors_start__ = .);
  KEEP(*(SORT(.dtors.*)))
  KEEP(*(.dtors))
  PROVIDE(__dtors_end__ = .);
  . = ALIGN(0x8) ;
  __usbh_class_info_start__ = .;
  KEEP(*(.usbh_class_info))
  __usbh_class_info_end__ = .;
  . = ALIGN(0x40) ;
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA
 .ram.code : {
  . = ALIGN(0x8) ;
  __ram_code_start__ = .;
  __sdata = . ;
  __data_start__ = . ;
  *(.ram.code*)
  __ram_code_end__ = .;
  . = ALIGN(0x8) ;
 } > REGION_DATA AT > REGION_DATA
 .data : {
  . = ALIGN(0x4) ;
  data_start = . ;
  KEEP(*startup_gcc.o(*.vectors*))
  *(.got.plt)
  *(.got)
  *(.gnu.linkonce.r*)
  *(.data)
  *(.data*)
  *(.data1)
  *(.data.*)
  *(.gnu.linkonce.d*)
  *(.data1)
  *(.gcc_except_table)
  *(.gcc_except_table*)
  __start_init_call = .;
  *(.initcall.init)
  __stop_init_call = .;
  __start_cmd = .;
  *(.bootloaddata.cmd)
  . = ALIGN(4) ;
  __stop_cmd = .;
  __global_pointer$ = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(__libc_atexit)
  *(__libc_subinit)
  *(__libc_subfreeres)
  *(.note.ABI-tag)
  __edata = .;
  __data_end__ = .;
  . = ALIGN(0x4) ;
 } > REGION_DATA AT > REGION_DATA
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_DATA AT > REGION_DATA
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __data_end__ = .;
 } > REGION_DATA AT > REGION_DATA
 .bss : {
  . = ALIGN(0x4) ;
  __sbss = ALIGN(0x4) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x4) ;
  __ebss = . ;
  __end = . ;
  end = . ;
  __bss_end__ = .;
 } > REGION_BSS
 ._sram_s0_sw_heap : {
  . = ALIGN(0x4) ;
  __sram_s0_sw_heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x4) ;
 } > REGION_SRAM_S0_SW AT > REGION_SRAM_S0_SW
 .sram_s1_sw : {
  . = ALIGN(0x4) ;
  __sram_s1_sw_data_start = .;
  *(.sram1_sw_data)
  . = ALIGN(0x4) ;
  __sram_s1_sw_data_end = .;
  __sram_s1_sw_heap_start = .;
 } > REGION_SRAM_S1_SW AT > REGION_SRAM_S1_SW
 .sram_s1_cma : {
  . = ALIGN(0x4) ;
  __sram_s1_cma_data_start = .;
  *(.sram1_cma_data)
  . = ALIGN(0x4) ;
  __sram_s1_cma_data_end = .;
  __sram_s1_cma_heap_start = .;
 } > REGION_SRAM_S1_CMA AT > REGION_SRAM_S1_CMA
}
