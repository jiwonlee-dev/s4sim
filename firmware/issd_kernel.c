 

#include <hw_drivers.h>
#include "issd_kernel.h"


extern char _firmware_end_;
extern char stack_end ;
extern unsigned char *uncached_ram_start ;
extern unsigned char *uncached_ram_end;


unsigned char * print_device = (unsigned char *)0x1C090000 ; 

ssd_config_t ssd_config ;

unsigned char *uncached_ram_start =  ( unsigned char *)( 2496 << 20) ;
unsigned char *uncached_ram_end;
unsigned char *uncacheable_next_ptr =  ( unsigned char *)( (2496 << 20) );

unsigned int sec_cpu_halt_indicator[4] = {0,0,0,0};

// TODO, to be managed Temp thread stack
#define thrd_stack_size  (1<<14)
u8 thrd_stack[8][thrd_stack_size];

thrd_t 		thrds[8];
uint32_t 	num_of_thrds = 1;  // 0 is default kernel thread
thrd_ftn_t	all_thrds_ftns[64];
uint32_t 	num_of_ftns = 0;
obj_queue_t queues[16];
int completed_req_cbk_ftn_id;
int process_req_cbk_ftn_id;



bool printing_initialized = false;
/*
 ARMv7 MMU TTB
 Generate a flat translation table for the whole address space.
 4,096 of 1MB sections from 0x000xxxxx to 0xFFFxxxxx
 
 0x000xxxxx ~~ 0x7ffxxxxx  :  non-shareable device memory
 0x800xxxxx ~~ 0x9ffxxxxx  :  normal memory 
 0xa00xxxxx ~~ 0xfffxxxxx	 :  non-shareable device memory
 
 0x00002c02 dev
 0x00007c0e normal
 
 */
#define DEV_MEM 0x00002c02
#define CACHEABLE_MEM 0x00007c0e
#define UNCACHEABLE_MEM 0x00002c02

void make_mmu_ttb(unsigned int cpu_num , unsigned int * ttb_base_address){
	
	unsigned int * table_address = ttb_base_address ;
	unsigned int table_entry ;
	
	printToDev	("\n############      INIT TTB       ############");	
	
	// device mem 
	for (unsigned int i = 0 ; i < 2048 ; i ++){
		table_entry = i << 20 ;  
		table_entry  = table_entry | DEV_MEM ;  
		table_address[i]  = table_entry  ;
	}
	
	// cacheable ram region
	for (unsigned int i = 2048 ; i < 2264 ; i ++){
		table_entry = i << 20 ;  
		table_entry  = table_entry | CACHEABLE_MEM ;
		table_address[i]  = table_entry  ;
	}
	 
	// uncacheable ram region 
	uncached_ram_start = ( unsigned char *)( 2264 << 20) ;
	uncacheable_next_ptr = uncached_ram_start;
	for (unsigned int i = 2264 ; i < 2560 ; i ++){
		table_entry = i << 20 ;  
		table_entry  = table_entry | UNCACHEABLE_MEM ; 
		table_address[i]  = table_entry ;
	}
	uncached_ram_end = (unsigned char *)( (2560 - 1) << 20 );
	
	// device region 
	for (unsigned int i = 2560 ; i < 4096 ; i ++){
		table_entry = i << 20 ;  
		table_entry  = table_entry | DEV_MEM ; 
		table_address[i]  = table_entry ;
	}
	
	*print_device = '.' ;
	
	return ;
}

uint32_t allocation_count = 0;
unsigned char * get_uncacheable_ram(unsigned int size){
	unsigned char * rtn = uncacheable_next_ptr;
	uncacheable_next_ptr += size ;
	
	//if (printing_initialized){
	//	VPRINTF_ERROR (" memory allocation : size:%u , call count:%u" , size , allocation_count);
	//}
	
	if (uncacheable_next_ptr > uncached_ram_end){
		
		if (printing_initialized){
			VPRINTF_ERROR (" *** ERROR : memory allocation *** size:%u , call count:%u" , size , allocation_count);
		}else{
			printToDev(" *** ERROR : memory allocation *** ");
		}
		shutDownSystem();
	}
	
	allocation_count++;
	return rtn ;
}


void halt_sec_cpus(unsigned int cpuID){
	while (sec_cpu_halt_indicator[cpuID] != MMU_TTB_READY) {}
	sec_cpu_halt_indicator[cpuID] = CPU_RUNNING ;
	return ;
}

void release_sec_cpus(){
	
	unsigned int time_out = 10000;
 
	sec_cpu_halt_indicator[1] = MMU_TTB_READY ;
	sec_cpu_halt_indicator[2] = MMU_TTB_READY ;
	sec_cpu_halt_indicator[3] = MMU_TTB_READY ;
	
	while (1) {
		
		if (sec_cpu_halt_indicator[1] == CPU_RUNNING && 
			sec_cpu_halt_indicator[2] == CPU_RUNNING &&
			sec_cpu_halt_indicator[3] == CPU_RUNNING ){
			return ;
		}
		
		time_out--;
		
		if(!time_out){
			printToDev("\nError releasing secondary CPUs");
		}
	}
	
}


int sys_create_thread ( int thrd){
	
	// check id 
	if ( thrd != num_of_thrds) {
		VPRINTF_ERROR (" %s : illegal thread id :%d " , __FUNCTION__ , thrd) ;
		shutDownSystem();
		return -1 ;
	}
	
	thrd_t *t = &thrds[num_of_thrds];
	t->id = num_of_thrds ;
	t->num_of_ftns = 0 ;
	t->num_of_queues = 0;
	t->ftn_cbk.head = 0;
	t->ftn_cbk.tail = 0;
	
	// set stack area
	t->stack_start = &(thrd_stack[num_of_thrds][thrd_stack_size - 4]) ;
	
	num_of_thrds++;
	return (num_of_ftns-1);
}

int sys_add_thread_function( int thrd , void* ftn_ptr){
	
	if (thrd < 1 || thrd >= num_of_thrds) {
		VPRINTF_ERROR (" %s : illegal thread id :%u " , __FUNCTION__ , thrd) ;
		shutDownSystem();
		return 0 ;
	}
	
	uint32_t ftn_id = num_of_ftns ;
	all_thrds_ftns[ftn_id].id = ftn_id;
	all_thrds_ftns[ftn_id].thrd_id = thrd;
	all_thrds_ftns[ftn_id].ftn_ptr = ftn_ptr;
	num_of_ftns++;
	
	thrds[thrd].function_ids[thrds[thrd].num_of_ftns] = ftn_id ;
	thrds[thrd].num_of_ftns++;
	
	return ftn_id;
}

int sys_create_obj_queue (int queue_id , int push_thrd , int cbk_ftn_id ){
	
	queues[queue_id].id = queue_id ;
	queues[queue_id].push_thrd = push_thrd ;
	queues[queue_id].cbk_ftn_id = cbk_ftn_id ;
	//queues[queue_id].cbk_ftn_thrd = all_thrds_ftns[cbk_ftn_id].thrd_id ;
	
	// init fifo queue
	queues[queue_id].tail = 0;
	queues[queue_id].head = 0;
	
	thrd_t *t = & thrds[all_thrds_ftns[cbk_ftn_id].thrd_id];
	t->msg_queues[t->num_of_queues] = queue_id ;
	t->num_of_queues ++;
	
	return 0 ;
}

//static inline 
int __push_obj_to_queue(int queue, void * obj_ptr, uint32_t obj_type ){
	
	if(queues[queue].head == (queues[queue].tail + 1) % MSG_Q_SIZE){
		// queue if full 
		VPRINTF_ERROR ("%s msg queue %u is full " , __FUNCTION__ );
		// TODO place is a temp area
		return -1;
	}
	
	queues[queue].entries[queues[queue].tail].obj_ptr = obj_ptr;
	queues[queue].entries[queues[queue].tail].obj_type = obj_type ;
	// advance tail 
	queues[queue].tail ++;
	queues[queue].tail = queues[queue].tail % MSG_Q_SIZE ;
	
	//VPRINTF_ERROR("KERNEL{%s}: q:%d obj:0x%x type:%u ", __FUNCTION__, queue , (uint32_t)obj_ptr, obj_type );
	
	return 0;
}

//static inline
int __pop_obj_from_queue(int queue ){
	
	if (queues[queue].head == queues[queue].tail ){
		// queue is empty 
		return -1 ;
	}
	
	int entry_index = queues[queue].head ;
	// advance head 
	queues[queue].head ++;
	queues[queue].head = queues[queue].head % MSG_Q_SIZE ;
	
	return entry_index;
}

//static inline
int __push_callback_ftn(int ftn_id, uint32_t arg_0){
	
	uint32_t thrd = all_thrds_ftns[ftn_id].thrd_id ;
	thrd_t *t = &thrds[thrd];
	
	//VPRINTF_ERROR("%s : %d %d %u ", __FUNCTION__, thrd , ftn_id, arg_0 );
	
	if(t->ftn_cbk.head == (t->ftn_cbk.tail + 1) % MSG_Q_SIZE  ){
		// queue if full 
		VPRINTF_ERROR ("%s msg queue %u is full " , __FUNCTION__ );
		// TODO place is a temp area
		return -1;
	}
	
	t->ftn_cbk.entries[t->ftn_cbk.tail].ftn_id = ftn_id;
	t->ftn_cbk.entries[t->ftn_cbk.tail].argv[0] = arg_0 ;
	
	// advance tail 
	t->ftn_cbk.tail ++;
	t->ftn_cbk.tail = t->ftn_cbk.tail % MSG_Q_SIZE ;
	
	return 0;
}

//static inline
int __pop_callback_ftn(thrd_t *t){
	
	if (t->ftn_cbk.head == t->ftn_cbk.tail ){
		// queue is empty 
		return -1 ;
	}
	
	int entry_index = t->ftn_cbk.head;
	// advance head 
	t->ftn_cbk.head ++;
	t->ftn_cbk.head = t->ftn_cbk.head % MSG_Q_SIZE ;
	return entry_index;
}

int sys_push_obj_to_queue(int queue, void * obj_ptr, uint32_t obj_type ){
	return __push_obj_to_queue(queue,obj_ptr,obj_type); 
}

void 	sys_push_to_ftl(void* req, uint32_t req_type){
	__push_obj_to_queue(FTL_MODULE_IN_Q,req,req_type);
}

void 	sys_push_to_nvme(void* req, uint32_t req_type){
	__push_obj_to_queue(FTL_MODULE_OUT_Q,req,req_type);
}

int sys_add_callback_ftn(int ftn_id, uint32_t arg_0){
	return __push_callback_ftn(ftn_id,arg_0);
}


// these are called by the primary cpu
// declare and call all necessary init functions
// functions must be defined in thier respective module code
extern int init_host_dma();
extern int init_nvme_driver();
extern int init_buffer_mgt();
extern int init_ftl_module();
extern int init_nand_io_module();
extern int init_isp_driver();

void init_firmware_ftn(unsigned int cpuID ){
	
	char tem_print_buffer [1024];
	
	unsigned int firmware_memsize = ((unsigned int)&_firmware_end_) - ((unsigned int )&_firmware_start_);
	firmware_memsize = firmware_memsize >> 10 ;
	
	unsigned int normal_heap_size = (unsigned int)uncached_ram_start - ((unsigned int)&_heap_start_) ;
	normal_heap_size = normal_heap_size >> 20 ;
	
	unsigned int uc_heap_size = (unsigned int)uncached_ram_end - (unsigned int )uncached_ram_start ;
	uc_heap_size = uc_heap_size >> 20 ;
	
	snprintf_(tem_print_buffer , sizeof(tem_print_buffer) , "\n Mem map : \
			  \n   Firmware start address :0x%x  %u \
			  \n   Firmware end           :0x%x  %u  Size:%uKB\
			  \n   Normal heap            :0x%x  %u  Size:%uMB \
			  \n   Uncachable heap start  :0x%x  %u  Size:%uMB " 
			  ,(unsigned int)&_firmware_start_ , (unsigned int)&_firmware_start_ 
			  ,(unsigned int)&_firmware_end_ , (unsigned int)&_firmware_end_ , firmware_memsize
			  ,(unsigned int)&_heap_start_ , (unsigned int)&_heap_start_ ,  normal_heap_size
			  ,(unsigned int)uncached_ram_start , (unsigned int)uncached_ram_start , uc_heap_size
			  );
	
	printToDev(tem_print_buffer);
	
	//stats_dev = (stats_output_t*) get_dev_address(DEV_STATS);
	//stats_dev->ds = HOST_DS ;	
	
	// buffers for in storage processing
	external_code_buffer = get_uncacheable_ram( ISP_MAX_APPLET_BIN_SIZE );
	
	init_printing();
	printing_initialized = true ;
	
	init_req_obj();
	
	// init other hardwares
	//extern int _main_(void);
	//_main_();
	

	// add base threads 
	sys_create_thread(HOST_DMA_THREAD);
	sys_create_thread(NVME_DRIVER_THREAD);
	sys_create_thread(FTL_MODULE_THREAD);
	//sys_create_thread(NAND_IO_THREAD);
	
	// call init functions
	init_isp_driver();
	init_host_dma();
	init_nand_io_module();
	init_nvme_driver();
	init_buffer_mgt();
	init_ftl_module();
	
	
	/*  main NVME <-> FTL queues setup */
	// queue for sending req to ftl module
	sys_create_obj_queue(FTL_MODULE_IN_Q , NVME_DRIVER_THREAD , process_req_cbk_ftn_id );  	
	// queue for sending completed req to nvme module
	sys_create_obj_queue(FTL_MODULE_OUT_Q , FTL_MODULE_THREAD , completed_req_cbk_ftn_id );	
	
	return;
}

cpu_threads_t thread_schedule[NUM_CPUS];

// TODO
u8 stack_reset[NUM_CPUS][(1<<13)] ;


void static inline assign_thrd_to_cpu( u32 thread_index , cpu_threads_t *cpu_thrds ){
	thrd_t *t = &thrds[thread_index];
	t->status = THREAD_STATUS_READY ;
	t->skipped_count = 0;
	cpu_thrds->threads[cpu_thrds->num_assigned_threads] = t;
	cpu_thrds->num_assigned_threads ++;
}


void firmware_entry (unsigned int cpuID ){

	cpu_threads_t *cpu_thrds = &thread_schedule[cpuID];
	cpu_thrds->cpu_id = cpuID ;
	cpu_thrds->num_assigned_threads = 0;
	cpu_thrds->current_thread = NULL;
	cpu_thrds->next_thread = NULL;
	cpu_thrds->stack_reset = &(stack_reset[cpuID][(1<<13) - 4]);


#if (NUM_CPUS == 1)
	VPRINTF_ERROR (" No thread for CPU %u", cpuID );
	while (1) {}

#elif (NUM_CPUS == 2)
	VPRINTF_ERROR (" No thread for CPU %u", cpuID );
	while (1) {}

#elif (NUM_CPUS == 3)
	VPRINTF_ERROR (" No thread for CPU %u", cpuID );
	while (1) {}		

#elif (NUM_CPUS == 4)
	
	if (!cpuID ){ 
		assign_thrd_to_cpu(NVME_DRIVER_THREAD , cpu_thrds);
		cpu_scheduler ( cpu_thrds , cpu_thrds->stack_reset );
	}else if(cpuID == 1){	
		assign_thrd_to_cpu(HOST_DMA_THREAD , cpu_thrds);
		cpu_scheduler ( cpu_thrds , cpu_thrds->stack_reset);
	}else if(cpuID == 2){	
		assign_thrd_to_cpu(FTL_MODULE_THREAD , cpu_thrds);
		cpu_scheduler ( cpu_thrds , cpu_thrds->stack_reset);
	}else if(cpuID == 3){	
		assign_thrd_to_cpu(ISP_THREAD_START_INDEX , cpu_thrds);
		cpu_scheduler ( cpu_thrds , cpu_thrds->stack_reset);
	}
#endif

	else{
		VPRINTF_ERROR (" No thread for CPU %u", cpuID );
		while (1) { }
	}

	return  ;
}
 
void run_thread(cpu_threads_t * cpu_threads , thrd_t * t ){
	
	int entry_index ;
	// check function call back queue
	entry_index = __pop_callback_ftn(t);
	
	if ( entry_index == -1 ){
		// empty
	}else {
			
		void* ftn_ptr = all_thrds_ftns[t->ftn_cbk.entries[entry_index].ftn_id].ftn_ptr;
		u32 arg_0 = t->ftn_cbk.entries[entry_index].argv[0];
		void (*f)(u32, u32) = (void (*)(u32, u32)) ftn_ptr;
		//VPRINTF_CPU_SCHEDULE(" obj cbk: t:%d, ftn_ptr:0x%x, arg_0:0x%x, arg_1:0x%x ", t->id , ftn_ptr, arg_0, 0);
		f(arg_0, 0 );
	}
		
	// check all object queues associated with this thread
	for (int i = 0 ; i < t->num_of_queues ; i++ ){
		
		u32 qid = t->msg_queues[i];	
		entry_index = __pop_obj_from_queue(qid);
		
		if (entry_index == -1 ){
			// empty
		}else {
			
			obj_queue_t *q = &(queues[qid]);
			int ftn_id = q->cbk_ftn_id ;
			void* ftn_ptr = all_thrds_ftns[ftn_id].ftn_ptr;
			void* obj_ptr = q->entries[entry_index].obj_ptr ;
			u32 obj_type = q->entries[entry_index].obj_type ;
				
			VPRINTF_CPU_SCHEDULE(" obj cbk: t:%d, ftn_ptr:0x%x, arg_0:0x%x, arg_1:0x%x ", t->id , ftn_ptr, obj_ptr, obj_type);
			void (*f)(u32, u32) = (void (*)(u32, u32)) ftn_ptr;
			f((u32)obj_ptr, obj_type);
		}
	}
	
	t->status = THREAD_STATUS_READY ;
	
	cpu_scheduler(cpu_threads, cpu_threads->stack_reset);
}


void sys_yield_cpu(){
	u32 cpu_id = get_cpu_id();
	cpu_threads_t *cpu_threads = &thread_schedule[cpu_id] ;
	cpu_threads->current_thread->status = THREAD_STATUS_SWPM ;
	
	asm_save_context(cpu_threads, &cpu_threads->current_thread->saved_stack_reg );
	
}

void cpu_thread_scheduler ( cpu_threads_t * cpu_threads){
	
	u32 t_index = 0;
	thrd_t *t;
	
	for (u32 i = 0 ; i < cpu_threads->num_assigned_threads; i++){
		
		//VPRINTF_CPU_SCHEDULE (" cpu_threads->current_thread:0x%x  ,  cpu_threads->threads[i]:0x%x ",
		//					cpu_threads->current_thread , cpu_threads->threads[i] );
		
		if(cpu_threads->current_thread == cpu_threads->threads[i]){
			t_index = i + 1 ;
			//VPRINTF_CPU_SCHEDULE(" c thread index:%u" , i);
			break;
		}
	}
	
	// goto next thread
	for (u32 i = 0 ; i < cpu_threads->num_assigned_threads; i++){
	
		if (t_index >= cpu_threads->num_assigned_threads) t_index = 0;
		
		//VPRINTF_CPU_SCHEDULE(" next thread index:%u" , t_index);
		
		t = cpu_threads->threads[t_index] ;
		
		if (t->status == THREAD_STATUS_READY){
			// run thread call back functions	
			//VPRINTF_CPU_SCHEDULE(" run thread index:%u" , t_index);
			cpu_threads->current_thread = t;
			t->status = THREAD_STATUS_ACTIVE ;
			asm_run_thread(cpu_threads, t, t->stack_start);
		
		}else if (t->status == THREAD_STATUS_SWPM ){
			
			//VPRINTF_CPU_SCHEDULE (" SWPM thrd: index:%u, t->saved_stack_reg:0x%x" , t_index  , t->saved_stack_reg );
			// thread is in waiting_polling mode, skip it n times
			t->skipped_count ++;
			
			if (t->skipped_count >= 2){
				// restore the thread
				//VPRINTF_CPU_SCHEDULE (" restore SWPM thrd: index:%u, t->saved_stack_reg:0x%x", t_index  , t->saved_stack_reg );
				
				t->skipped_count = 0;
				cpu_threads->current_thread = t;
				t->status = THREAD_STATUS_ACTIVE ;
				asm_restore_saved_thread(t->saved_stack_reg);
			}
		}
		
		t_index ++;
	}
	
	/* TODO if no thread is ready, cpu can be set to sleep or low frequency mode */
	
}
 
// interrupt handling
void isr(unsigned int int_num)
{
	switch (int_num) {			
		default:
			break;
	}
}


void sys_host_shutdown(){
	shutdown_ftl_module();
	shutdown_nand_io_driver();
	shutdown_nvme_driver();
	// stop simulation
	//shutDownSystem();
}


/*
 * GEM5 shutdown the simulation when EOT is written to UART
 */
void shutDownSystem(){
	printToDev("\n SHUTDOWN MACHINE ");
	*print_device = 0x04 ;
	printToDev("\n should not get here ");
	while(1);
}

/*
 * return device register base address 
 */
unsigned char * get_dev_address( int DeviceID ){
	
	switch (DeviceID) 
	{
			case DEV_HOST_INT:
			return (unsigned char *)0x2d100000;
			break;
			
			case DEV_NAND_CTRL:
			return (unsigned char*)0x2d108000 ; 
			break ;
			
			case DEV_STATS:
			return (unsigned char *)0x2d419000 ; 
			
		default:
			break;
	}
	
	return (unsigned char *)0 ;
}

void printToDev(const char *s) {
	while(*s != '\0')
	{ 
		*print_device = (unsigned char)(*s); 
		s++; 
	}
}

void printCharToDev( unsigned char char_){
	*print_device = char_;
}



  




