
 
.section mmu_ttb_start
L1_MMU_TTB_START:
.align 14		
.skip  0x00004000

 
//GIC_Distributor
.equ GIC_Dist_Base,   0x2C001000 

//Register offsets
.equ set_enable1,       0x104
.equ set_enable2,       0x108

//Example definitions
.equ timer_irq_id,      36   // 36 <64 => set_enable1 Reg

//GIC_CPU_INTERFACE
.equ GIC_CPU_BASE,                  0x2C002000 
.equ GIC_CPU_mask_reg_offset,       0x04
.equ GIC_CPU_Int_Ack_reg_offset,    0x0C
.equ GIC_CPU_End_of_int_offset,     0x10


// ===========================================================
// global symbols needed by GEM5 linux platform code
// GEM5 searches for these symbols in the kernel file
// ===========================================================
.global panic 
panic:
	B .

.global oops_exit
oops_exit :
	B .

.global __udelay 
__udelay :
	B .

.global __const_udelay
__const_udelay :
	B .

.section ISSD_FIRMWARE_STARTUP, "x"
.global _Reset
_Reset:
	B start_up			  
	B .                /* Undefined */
	B .                /* SWI */
	B .                /* Prefetch Abort */
	B .                /* Data Abort */
	B .                /* reserved */
	B irq_handler      /* IRQ */
	B .                /* FIQ */


.align
.global start_up 
start_up:

// ===========================================================
// Set up Stack Pointers 
// for IRQ processor mode and SVC (supervisor) mode
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field

	mov		R1, #0b11010010					// interrupts masked, MODE = IRQ   IRQ | FIQ | 0 | Mode[4:0]
	msr		CPSR, R1					// change to IRQ mode
	LDR     r1, =irq_stack_end			// load irq stack base 
	SUB     r1, r1, r0, LSL #12			// 4K bytes of IRQ stack per CPU  
	MOV     sp, r1						// assign the pointer
	 
	mov		R1, #0b11010011					// interrupts masked, MODE = SVC   IRQ | FIQ | 0 | Mode[4:0]
	msr		CPSR, R1					// change to SVC mode
	LDR     r1, =stack_end				// load stack base address
	SUB     r1, r1, r0, LSL #14			// 16kb bytes of App stack per CPU  
	MOV     sp, r1						// assign the pointer

// ===========================================================
// Branch into Primary and Secondary CPU Initialization
// ===========================================================
	MRC     p15, 0, r0, c0, c0, 5		// Read CPU ID register
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	BEQ     primaryCPUInit
	BNE     secondaryCPUsInit
	B	.
// ===========================================================
// END of _Reset  
// ===========================================================



.align 
.global primaryCPUInit
primaryCPUInit:
// ===========================================================
// setup MMU TTB and enable MMU 
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	LDR     r1, =L1_MMU_TTB_START		
	BL		make_mmu_ttb
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	BL      init_firmware_ftn
	BL		release_sec_cpus
	BL		enableMMU_leaveCachesDisabled


// ===========================================================
// enable all caches 
// ===========================================================
	BL		enable_L1_D_side_prefetch
	BL		enable_L2_prefetch_hint
	BL		enable_caches

// ===========================================================
// Branch to cpu firmware C code main(0)
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	B       firmware_entry
	B		.							// we should never get here

// ===========================================================
// End of primaryCPUInit 
// ===========================================================



.align 
.global secondaryCPUsInit
secondaryCPUsInit:

// ===========================================================
// Hold secondary CPU until CPU 0 releases them : halt_sec_cpus(cpuid)
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	BL		halt_sec_cpus

// ===========================================================
// enable MMU , flat TTB already generated by cpu 0
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	LDR     r1, =L1_MMU_TTB_START		
	BL		enableMMU_leaveCachesDisabled

// ===========================================================
// enable all caches 
// ===========================================================
	BL	enable_L1_D_side_prefetch
	BL	enable_L2_prefetch_hint
	BL	enable_caches

// ===========================================================
// Branch to cpu firmware C code main(cpuid)
// ===========================================================
	MRC		p15, 0, r0, c0, c0, 5		// Read CPU ID register	
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	B       firmware_entry
	B		.							// we should never get here

// ===========================================================
// End of secondaryCPUsInit 
// ===========================================================



// ===========================================================
// retuen cpu ID 
// ===========================================================
.align
.global get_cpu_id
get_cpu_id:
	MRC     p15, 0, r0, c0, c0, 5		// Read CPU ID register
	ANDS    r0, r0, #0x03					// Mask off, leaving the CPU ID field
	BX      lr


// ===========================================================
//
// ===========================================================
.align
.global cpu_thread_scheduler
.global cpu_scheduler
cpu_scheduler:
	mov sp , r1
	ldr r2 , =cpu_thread_scheduler
	bx r2


// ===========================================================
//
// ===========================================================
.align
.global run_thread
.global asm_run_thread
asm_run_thread:
	mov sp , r2
	ldr r2 , =run_thread
	bx r2


// ===========================================================
//
// ===========================================================
.align
.global asm_save_context
asm_save_context:
	push {lr}
	push {r0-r12}
	mov  r2, sp
	str  r2, [r1]
	ldr  r3 , =cpu_thread_scheduler
	bx   r3

// ===========================================================
//
// ===========================================================
.align
.global asm_restore_saved_thread
asm_restore_saved_thread:
	mov	sp, r0
	pop	{r0-r12}
	pop	{pc}


// ===========================================================
//
// void run_callback_ftn(u32 arg_0, u32 arg_1, void* ftn_ptr, u32 thrd_stack_ptr );
// ===========================================================
.align
.global run_callback_ftn
run_callback_ftn:
	push 	{lr} //{r4, lr}
	//mov	r4, sp
	//mov	sp, r3			// set thread stack area
	blx	r2
	//mov	sp, r4
	pop	{pc} //{r4, pc}


// ===========================================================
//  IRQ Handler that calls the ISR function in C
// ===========================================================
.global irq_handler
irq_handler:
    push {r0-r7,lr}

    // Read the interrupt acknowledge register of the GIC_CPU_INTERFACE
    ldr r1, =GIC_CPU_BASE + GIC_CPU_Int_Ack_reg_offset
    ldr r2, [r1]

irq_unexpected:
    //cmp r2, #timer_irq_id
    //bne irq_unexpected  // if irq is not from timer0

    // Jump to C - must clear the timer interrupt!
	mov r0 , r2
    BL isr

    // write the IRQ ID to the END_OF_INTERRUPT Register of GIC_CPU_INTERFACE
    ldr r1, =GIC_CPU_BASE + GIC_CPU_End_of_int_offset
    ldr r2, =timer_irq_id
    str r2, [r1]

    pop {r0-r7,lr}
    subs pc, lr, #4



// ===========================================================
// Enable L1 D-side prefetch (A9 specific)
// ===========================================================
.align
.global enable_L1_D_side_prefetch
enable_L1_D_side_prefetch:
	MRC     p15, 0, r0, c1, c0, 1      // Read Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 2)			    // Set DP bit 2 to enable L1 Dside prefetch
	MCR     p15, 0, r0, c1, c0, 1      // Write Auxiliary Control Register
	BX      lr

	

// ===========================================================
// Enable L2 prefetch hint
// Note: The Preload Engine can also be programmed to improve L2 hit
// rates.  This may be too much work though.
// ===========================================================
.align
.global enable_L2_prefetch_hint
enable_L2_prefetch_hint:
	MRC     p15, 0, r0, c1, c0, 1		// Read Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 1)				// Set bit 1 to enable L2 prefetch hint
	MCR     p15, 0, r0, c1, c0, 1		// Write Auxiliary Control Register
	BX      lr



// ===========================================================
//  Enable caches and branch prediction
// ===========================================================
.align
.global enable_caches
enable_caches:
	MRC     p15, 0, r0, c1, c0, 0		// Read System Control Register
	ORR     r0, r0, #(0x1 << 12)				// Set I bit 12 to enable I Cache
	ORR     r0, r0, #(0x1 << 2)				// Set C bit  2 to enable D Cache
	ORR     r0, r0, #(0x1 << 11)				// Set Z bit 11 to enable branch prediction
	MCR     p15, 0, r0, c1, c0, 0		// Write System Control Register
	BX		lr



// ===========================================================
// Cortex-A9 MMU Configuration
// Set translation table base
//
// mmu ttb already populated by cpu 0
// ===========================================================
.align
.global enableMMU_leaveCachesDisabled
enableMMU_leaveCachesDisabled:
	PUSH	{r4, r5, r7, r9, r10, r11, lr}

//------------------------------------------------------------------
// Disable caches, MMU and branch prediction 
// in case they were left enabled from an earlier run
// This does not need to be done from a cold reset
//------------------------------------------------------------------
	MRC     p15, 0, r0, c1, c0, 0		// Read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)				// Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)				// Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x1					// Clear M bit  0 to disable MMU
	BIC     r0, r0, #(0x1 << 11)				// Clear Z bit 11 to disable branch prediction
	MCR     p15, 0, r0, c1, c0, 0		// Write value back to CP15 System Control register

//------------------------------------------------------------------
// Invalidate Data and Instruction TLBs and branch predictor
//------------------------------------------------------------------
	MOV     r0,#0
	MCR     p15, 0, r0, c8, c7, 0		// I-TLB and D-TLB invalidation
	MCR     p15, 0, r0, c7, c5, 6		// BPIALL - Invalidate entire branch predictor array

//------------------------------------------------------------------
// Cache Invalidation code for Cortex-A9
//------------------------------------------------------------------

// Invalidate L1 Instruction Cache
	MRC     p15, 1, r0, c0, c0, 1		//@ Read Cache Level ID Register (CLIDR)
	TST     r0, #0x3						//@ Harvard Cache?
	MOV     r0, #0						//@ SBZ
	MCRNE   p15, 0, r0, c7, c5, 0		//@ ICIALLU - Invalidate instruction cache and flush branch target cache

//@ Invalidate Data/Unified Caches
	MRC     p15, 1, r0, c0, c0, 1		@ Read CLIDR
	ANDS    r3, r0, #0x07000000				@ Extract coherency level
	MOV     r3, r3, LSR #23				@ Total cache levels << 1
	BEQ     Finished					@ If 0, no need to clean

	MOV     r10, #0                    @ R10 holds current cache level << 1
	Loop1:  ADD     r2, r10, r10, LSR #1       @ R2 holds cache "Set" position
	MOV     r1, r0, LSR r2             @ Bottom 3 bits are the Cache-type for this level
	AND     r1, r1, #7                 @ Isolate those lower 3 bits
	CMP     r1, #2
	BLT     Skip                       @ No cache or only instruction cache at this level

	MCR     p15, 2, r10, c0, c0, 0     @ Write the Cache Size selection register
	ISB                                @ ISB to sync the change to the CacheSizeID reg
	MRC     p15, 1, r1, c0, c0, 0      @ Reads current Cache Size ID register
	AND     r2, r1, #7                 @ Extract the line length field
	ADD     r2, r2, #4                 @ Add 4 for the line length offset (log2 16 bytes)
	LDR     r4, =0x3FF
	ANDS    r4, r4, r1, LSR #3         @ R4 is the max number on the way size (right aligned)
	CLZ     r5, r4                     @ R5 is the bit position of the way size increment
	LDR     r7, =0x7FFF
	ANDS    r7, r7, r1, LSR #13        @ R7 is the max number of the index size (right aligned)

	Loop2:  MOV     r9, r4                     @ R9 working copy of the max way size (right aligned)

	Loop3:  ORR     r11, r10, r9, LSL r5       @ Factor in the Way number and cache number into R11
	ORR     r11, r11, r7, LSL r2       @ Factor in the Set number
	MCR     p15, 0, r11, c7, c6, 2     @ Invalidate by Set/Way
	SUBS    r9, r9, #1                 @ Decrement the Way number
	BGE     Loop3
	SUBS    r7, r7, #1                 @ Decrement the Set number
	BGE     Loop2
	Skip:   ADD     r10, r10, #2               @ increment the cache number
	CMP     r3, r10
	BGT     Loop1

Finished:

//------------------------------------------------------------------
// Clear Branch Prediction Array
//------------------------------------------------------------------
	MOV     r0, #0
	MCR     p15, 0, r0, c7, c5, 6      @ BPIALL - Invalidate entire branch predictor array

//------------------------------------------------------------------
// Cortex-A9 MMU Configuration
// Set translation table base
//------------------------------------------------------------------

// ===========================================================
// Cortex-A9 supports two translation tables
// Configure translation table base (TTB) control register cp15,c2
// to a value of all zeros, indicates we are using TTB register 0.
// ===========================================================
	MOV     r0,#0x0
	MCR     p15, 0, r0, c2, c0, 2

	//@ write the address of our page table base to TTB register 0
	LDR     r0, =L1_MMU_TTB_START
	MOV     r1, #0x08	@ RGN=b01  (outer cacheable write-back cached, write allocate)

	//@ S=0      (translation table walk to non-shared memory)
	ORR     r1,r1,#0x40	@ IRGN=b01 (inner cacheability for the translation table walk is Write-back Write-allocate)

	ORR     r0,r0,r1                    
	MCR     p15, 0, r0, c2, c0, 0


init_ttb_1:

//@------------------------------------------------------------------=
//@ Setup domain control register - Set all domains to master mode
//@------------------------------------------------------------------=
	MRC     p15, 0, r0, c3, c0, 0      @ Read Domain Access Control Register
	LDR     r0, =0xFFFFFFFF				@ Initialize every domain entry to 0b11 (master)
	MCR     p15, 0, r0, c3, c0, 0      @ Write Domain Access Control Register

 
// ===========================================================
// Enable MMU, Leave the caches disabled
// ===========================================================
	MRC     p15, 0, r0, c1, c0, 0      @ Read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)       @ Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)       @ Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x2               @ Clear A bit  1 to disable strict alignment fault checking
	ORR     r0, r0, #0x1               @ Set M bit 0 to enable MMU before scatter loading
	MCR     p15, 0, r0, c1, c0, 0      @ Write CP15 System Control register

	POP	{r4, r5, r7, r9, r10, r11, pc}	@ pop the lr into the pc

// ===========================================================
// END of enableMMU_leaveCachesDisabled   
// ===========================================================
   


