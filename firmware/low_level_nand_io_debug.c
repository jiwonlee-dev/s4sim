 

#include "low_level_nand_io.h"

#define SYNC_NAND_IO_Q	11

typedef struct Virtual_NAND_t {
	uint8_t page[NUM_CHANNELS] 
	[DIES_PER_CHANNEL] 
	[PLANES_PER_DIE]
	[BLOCKS_PER_PLANE]
	[PAGES_PER_BLOCK] 
	[NAND_PAGE_SIZE] ;
}vn_t;
vn_t  * virtual_nand ;


int init_nand_io_module(){
	
	int ftn_id;
	ftn_id = sys_add_thread_function( FTL_MODULE_THREAD , initialize_nand_ctrl);
	sys_add_callback_ftn(ftn_id, 0 );
	
	init_op_obj();
	
	virtual_nand = (vn_t*) get_uncacheable_ram (sizeof(vn_t)) ;	
	VPRINTF_CONFIG(" virtual page size %u MB " , sizeof(vn_t) >> 20 );
	
	return 0;
}

void initialize_nand_ctrl (uint32_t arg1){
	VPRINTF_CONFIG	("############    INIT NAND CTRL   ############");
}

void __do_sync_flash_read(rop_t *op){
	
	if(!op->nop ) {return ; }
	
	ppa_t ppa ;
	for (uint32_t ix = 0 ; ix < op->nop ; ix++){
		
		ppa.data = op->pages[ix].ppa.data ;
		u8* src_ptr = (u8*)&(virtual_nand->page
								   [ppa.path.chn] 
								   [ppa.path.die]
								   [ppa.path.plane]
								   [ppa.path.block] 
								   [ppa.path.page ][0]) ;
		
		u8* dst_ptr = op->pages[ix].ssd_addr;
		
		u64* src = (u64*)src_ptr;
		u64* dst = (u64*)dst_ptr;		
		for (u32 ix = 0 ; ix < NAND_PAGE_SIZE / 8 ; ix++ ){ dst[ix] = src[ix] ; }
	}
	
	return ;
}

void __do_sync_flash_program(pop_t* op){
	
	if(!op->nop ){ return ;}
	
	ppa_t ppa ;
	
	for (uint32_t ix = 0 ; ix < op->nop ; ix++){
		
		ppa.data = op->pages[ix].ppa.data ;
		u8* src_ptr = op->pages[ix].ssd_addr  ;
		u8* dst_ptr = (u8*)&(virtual_nand->page 
								   [ppa.path.chn] 
								   [ppa.path.die]
								   [ppa.path.plane]
								   [ppa.path.block] 
								   [ppa.path.page ][0]) ;
		u64* src = (u64*)src_ptr;
		u64* dst = (u64*)dst_ptr;		
		for (u32 ix = 0 ; ix < NAND_PAGE_SIZE / 8 ; ix++ ){ dst[ix] = src[ix] ; }
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

void add_read_op_page(void* __rop , ppa_data_t ppa , unsigned char * ssd_addr , void * callback){
	rop_t* op = (rop_t*)__rop;
	op->pages[op->nop].ppa.data = ppa;
	op->pages[op->nop].ssd_addr = ssd_addr;
	op->nop++;
}

int sync_end_read_op(void* __rop ){
	rop_t* op = (rop_t*)__rop;
	__do_sync_flash_read(op);
	free_op_obj(__rop);
	return 0;
}

void* begin_program_op(bool pg_cbk){
	pop_t* pop = malloc_op_obj(FLASH_PROGRAM_OPERATION);
	return (void*)pop ;
}

void add_program_op_page(void* __pop, ppa_data_t ppa , unsigned char * ssd_addr , void * callback){
	pop_t* op = (pop_t*)__pop;
	op->pages[op->nop].ppa.data = ppa;
	op->pages[op->nop].ssd_addr = ssd_addr;
	op->nop++;
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





 


 

