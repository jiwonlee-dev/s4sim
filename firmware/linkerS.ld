 


ENTRY(_Reset)
SECTIONS
{
	. = 0x80000000;
	PROVIDE (_firmware_start_ = .) ;
	.text : {
		*(ISSD_FIRMWARE_STARTUP)
		*(.text)
	}
  
	. = ALIGN(8);
	PROVIDE (_mapping_table_start_ = .);
	mapping_tables : { *(mapping_tables) }
	PROVIDE (_mapping_table_end_ = .);
	
	. = ALIGN(8);
	.data : { *(mmu_ttb_start) *(.data) }


	.bss : { *(.bss COMMON) }
	. = ALIGN(8);

	PROVIDE (_firmware_end_ = .)   ;

	irq_stack_base = .;
	. = . + 0x4000;						/* 16kB of stack memory for IRQ , 4k per cpu core */
	irq_stack_end = .;
	
	stack_base = . ;
	. = . + 0x10000;					/* 64kB of stack memory , 16kB per cpu core */
	stack_end = . ;

	PROVIDE (_stack_end_ = .)   ;
	PROVIDE (_heap_start_ = .)   ;
	PROVIDE (end = .)   ;

}
