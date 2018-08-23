 

#include "low_level_nand_io.h"

#undef setReg
#define setReg(dev,data) dev = data ;

#undef getReg
#define getReg(dev,data) data=dev ;

#define NUM_NAND_CTRLS (NUM_CHANNELS / 2)

#define SYNC_NAND_IO_Q	11

unsigned char * ctrl_base_addr[NUM_NAND_CTRLS];
unsigned char * chn_bass_addr[NUM_CHANNELS];

int init_nand_io_module(){
	
	VPRINTF_STARTUP ( "NUM_NAND_CTRLS : %u  ", NUM_NAND_CTRLS);
	
	unsigned char* base_addr = get_dev_address(DEV_NAND_CTRL);
	
	int ichn = 0; int ictrl = 0;
	for (; ictrl < NUM_NAND_CTRLS ; ictrl ++){
		ctrl_base_addr[ictrl] = base_addr + (ictrl * 0x1000) ;
		chn_bass_addr[ichn++] = ctrl_base_addr[ictrl] + CHANNEL_0_BASE_ADDR ;
		chn_bass_addr[ichn++] = ctrl_base_addr[ictrl] + CHANNEL_1_BASE_ADDR ;
	}
	
	int ftn_id;
	ftn_id = sys_add_thread_function( FTL_MODULE_THREAD , initialize_nand_ctrl);
	sys_add_callback_ftn(ftn_id, 0 );

	 
	init_op_obj();
	
	return 0;
}

volatile nandCtrlReg * nandctrl_base_reg;

void initialize_nand_ctrl (uint32_t arg1){

	VPRINTF_STARTUP	("############    INIT NAND CTRL   ############");
	
	for (uint32_t ictrl = 0 ; ictrl < NUM_NAND_CTRLS ; ictrl++){
		
		nandctrl_base_reg = (nandCtrlReg * ) ctrl_base_addr[ictrl];
		VPRINTF_STARTUP ("nand ctrl %u base address 0x%x " , ictrl,  nandctrl_base_reg);

		// Send init command to NFC and get controller configuration
		nandctrl_base_reg->cmd = 0x00000010 ;
		
		// wait for completion
		reg cmdStatus ;
		while (1){
			cmdStatus = nandctrl_base_reg->cmdStatus;
				
			if (cmdStatus == COMPLETE  ) break ;
			
		}
		
		VPRINTF_STARTUP ( "nand ctrl %u init complete" , ictrl );
	}
}

int shutdown_nand_io_driver(){
	
	volatile nandCtrlReg * nandctrl_base_reg = (nandCtrlReg * ) get_dev_address(DEV_NAND_CTRL);
	setReg( nandctrl_base_reg->cmd , NAND_CTRL_SHUTDOWN )  ;
	// wait for completion
	reg cmdStatus ;
	while (1){
		getReg( nandctrl_base_reg->cmdStatus , cmdStatus)
		if (cmdStatus == COMPLETE  ) break ;
		sys_yield_cpu();
	}
	return 0;
}


void __do_sync_flash_read(rop_t *op){
	
	uint32_t chip_index ;
	uint32_t chn_start ;
	uint32_t chn_end ;	
	uint32_t cmdStatus;

	if(!op->nop ) {return ; }
	
	ppa_t ppa ;
	for (uint32_t ix = 0 ; ix < op->nop ; ix++){
		
		ppa.data = op->pages[ix].ppa.data ;
		
		// send read command
		// find channel address
		unsigned char* base_addr = chn_bass_addr[ppa.path.chn];
		
		address_cycle_t addr_cycle ;
		addr_cycle.data = 0 ;
		addr_cycle.shorts.short_1 = ppa.path.page;
		addr_cycle.shorts.short_2 = ppa.path.block;
		chip_index = (ppa.path.die * PLANES_PER_DIE) + ppa.path.plane ;
		
		VPRINTF_NAND_IO( "%s: %s:%u %s:%u %s:%u %s:%u ", 
			"READ NAND", 
			"Channel" , (u32)ppa.path.chn ,
			"Chip Index", (u32)chip_index ,
			"Block" , (u32)ppa.path.block,
			"Page", (u32)ppa.path.page ) ;

		// command phase
		*((reg *)(base_addr + CHIP_SELECT_REG ))		= chip_index ; 
		*((reg *)(base_addr + ADDRESS_CYCLE_REG_1 ))	= addr_cycle.addr.addr_1 ;
		*((reg *)(base_addr + ADDRESS_CYCLE_REG_2 ))	= 0 ;
		*((reg *)(base_addr + CMD_PHASE_CMD_REG ))		= READ_NAND;
		
		// data phass
		*((reg *)(base_addr + CHIP_SELECT_REG )) = chip_index;
		*((reg *)(base_addr + DMA_RAM_PTR )) 	 = (reg)op->pages[ix].ssd_addr; 		
		*((reg *)(base_addr + DMA_PHASE_CMD_REG )) = WRITE_DMA;
		
	}
	
	// check status
	chn_start = 0;
	chn_end = NUM_CHANNELS - 1;
	
	while (1){
		
		cmdStatus = 0;
		
		for (uint32_t ichn = chn_start ; ichn <= chn_end ; ichn++ ){
			
			cmdStatus += ((reg*)(chn_bass_addr[ichn] + ACTIVE_OPERATION_COUNT ))[0] ;
		}
		
		if (! cmdStatus) break ;
		
		sys_yield_cpu();
	}
	
	return ;
}
void __do_sync_flash_program(pop_t* op){
	
	if(!op->nop ){ return ;}
	
	ppa_t ppa ;
	
	for (uint32_t ix = 0 ; ix < op->nop ; ix++){
		
		ppa.data = op->pages[ix].ppa.data ;
		
		// send read command
		// get channel address
		unsigned char* base_addr = chn_bass_addr[ppa.path.chn];
		
		address_cycle_t addr_cycle  ;
		addr_cycle.data = 0 ;
		addr_cycle.shorts.short_1 = ppa.path.page;
		addr_cycle.shorts.short_2 = ppa.path.block;
		uint32_t chip_index = (ppa.path.die * PLANES_PER_DIE) + ppa.path.plane ;
		
		VPRINTF_NAND_IO( "%s: %s:%u %s:%u %s:%u %s:%u ", 
			"PROGRAM NAND", 
			"Channel" , (u32)ppa.path.chn ,
			"Chip Index", (u32)chip_index ,
			"Block" , (u32)ppa.path.block,
			"Page", (u32)ppa.path.page ) ;


		// command phase
		//select chip
		*((reg *)(base_addr + CHIP_SELECT_REG ))	 = chip_index ; 
		*((reg *)(base_addr + ADDRESS_CYCLE_REG_1 )) = addr_cycle.addr.addr_1 ;
		*((reg *)(base_addr + ADDRESS_CYCLE_REG_2 )) = 0 ;
		*((reg *)(base_addr + CMD_PHASE_CMD_REG ))	 = PROGRAM_NAND;
		
		// data phase
		*((reg *)(base_addr + CHIP_SELECT_REG ))	= chip_index;
		*((reg *)(base_addr + DMA_RAM_PTR ))		= (reg)op->pages[ix].ssd_addr ; 
		*((reg *)(base_addr + DMA_PHASE_CMD_REG ))	= READ_DMA;
		
	}
	
	// check status
	uint32_t chn_start = 0;
	uint32_t chn_end = NUM_CHANNELS-1;
	
	uint32_t cmdStatus;
	
	while (1){
		
		cmdStatus = 0;
		
		for (uint32_t ichn = chn_start ; ichn <= chn_end ; ichn++ ){
			
			cmdStatus += ((reg*)(chn_bass_addr[ichn] + ACTIVE_OPERATION_COUNT ))[0] ;
		}
		
		if (! cmdStatus) break ;
		sys_yield_cpu();
	}
	
	return ;
}


void sync_nand_io_cbk(void* io , uint32_t io_type) {
	
}

void* begin_read_op(bool pg_cbk){
	rop_t* rop = malloc_op_obj(FLASH_READ_OPERATION) ;
	rop->nop = 0;
	return (void*)rop ;
}

uint32_t add_read_op_page(void* __rop , ppa_data_t ppa , unsigned char * ssd_addr , void * callback){
	rop_t* op = (rop_t*)__rop;
	
	if (op->nop >= 128){
		VPRINTF_ERROR (" *** Can not add read operation , No. Pages:%u *** ", op->nop ) ;
		return 0;
	}

	op->pages[op->nop].ppa.data = ppa;
	op->pages[op->nop].ssd_addr = ssd_addr;
	op->nop++;
	return op->nop ;
}

int sync_end_read_op(void* __rop ){
	rop_t* op = (rop_t*)__rop;
	__do_sync_flash_read(op);
	free_op_obj(__rop);
	return 0;
}

void* begin_program_op(bool pg_cbk){
	pop_t* pop = malloc_op_obj(FLASH_PROGRAM_OPERATION);
	pop->nop = 0 ;
	return (void*)pop ;
}

uint32_t add_program_op_page(void* __pop, ppa_data_t ppa , unsigned char * ssd_addr , void * callback){
	pop_t* op = (pop_t*)__pop;

	if (op->nop >= 128){
		VPRINTF_ERROR (" *** Can not add program operation , No. Pages:%u *** ", op->nop ) ;
		return 0;
	}

	op->pages[op->nop].ppa.data = ppa;
	op->pages[op->nop].ssd_addr = ssd_addr;
	op->nop++;
	return op->nop;
}

int sync_end_program_op(void* __pop ){
	pop_t* op = (pop_t*)__pop;
	__do_sync_flash_program(op);
	free_op_obj(__pop);
	return 0;
}

#define MAX_OPS 128
typedef struct op_obj_queue{
	int tail;
	int head;
	int size;
	void *objs[MAX_OPS];
}op_q_t ;

rop_t		rops[MAX_OPS];
pop_t		pops[MAX_OPS];

op_q_t 	free_rop_objs ;
op_q_t 	free_pop_objs ;

void init_op_obj(){
	op_q_t * q;
	
	free_rop_objs.size = MAX_OPS;
	free_pop_objs.size = MAX_OPS;
	 
	q = &free_rop_objs;
	q->tail = q->head = 0 ;
	for (int i = 0 ; i < q->size ; i++){
		rops[i].type = FLASH_READ_OPERATION ;
		rops[i].index = i+1;
		q->objs[i] = (void*)&rops[i] ;
	}
	q->tail = q->size; q->tail = q->tail % q->size ;
	
	q = &free_pop_objs;
	q->tail = q->head = 0 ;
	for (int i = 0 ; i < q->size ; i++){
		pops[i].type = FLASH_PROGRAM_OPERATION ;
		pops[i].index = i+1;
		q->objs[0] = (void*)&pops[i] ;
	}
	q->tail = q->size; q->tail = q->tail % q->size ;

}

void * malloc_op_obj(uint32_t req_type){
	
	op_q_t * q;
	
	switch (req_type) {
			
		case FLASH_READ_OPERATION:
			q = &free_rop_objs;
			break;
		case FLASH_PROGRAM_OPERATION:
			q = &free_pop_objs;
			break;
		default:
			return (void*)0;
			break;
	}
	
	void *op = q->objs[q->head]; 
	q->head++ ; 		
	q->head = q->head % q->size ; 
	return op;
}

void free_op_obj(void * op_obj){
	op_q_t *q;
	uint32_t type = *((uint32_t*)op_obj);
	
	switch (type) {
			
		case FLASH_READ_OPERATION:
			q = &free_rop_objs;
			break;
		case FLASH_PROGRAM_OPERATION:
			q = &free_pop_objs;
			break;
		default:
			return;
			break;
	}
	
	q->objs[q->tail] = op_obj ;
	q->tail++; 
	q->tail = q->tail % q->size ; 
}






