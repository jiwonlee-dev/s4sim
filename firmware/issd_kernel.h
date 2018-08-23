 

#define KERNEL_THREAD	0
#define MSG_Q_SIZE  	256

enum {
	MMU_TTB_READY	= 1,
	CPU_RUNNING		= 2
};

struct thread_schedule_queue{
	
	uint32_t schedule_type ; 	// type_of_schedule : e.g. 1: pop queue , 2 direct function with 1 arg, 3 direct function call with 2 args 
	uint32_t id ; 				// queue or function id
	uint32_t arg1 ;
	uint32_t arg2 ; 	
};


#define THREAD_STATUS_ACTIVE	0x1
#define THREAD_STATUS_READY	0x2
#define THREAD_STATUS_SWPM		0x3 // SWPM	- context Saved Waiting with Polling Mode


typedef struct thread_t {
	
	u32 	saved_stack_reg ;
	
	u32 	id ;
	u32		status ;
	u32	   	skipped_count;
	u8* 	stack_start;
	
	u32 	current_cpu_core ;
	u32 	current_exec_ftn_id;
	
  	u32 	num_of_ftns ;
	u32 	function_ids[16];
	
	u32 	num_of_queues;
	u32 	msg_queues[16];
	

	// using FIFO for internal function callback queue
	struct {
		int tail;
		int head;
		struct {
			int  	ftn_id;
			u32		argv[4]; 	// argument values
			int 	argc;		// argument count
  		}entries[MSG_Q_SIZE] ;
	
	}ftn_cbk ;	
	
	// saved cpu state
	struct {
		u32 registers[18];
	} cpu_state ;
	
} thrd_t ;

typedef struct thread_function_t{
	
	u32 	id ;
	void 	*ftn_ptr ;
	u32 	thrd_id;
	
} thrd_ftn_t;

typedef struct obj_queue_t{
	
	u32 	id ;
	u32 	push_thrd ;
	u32 	cbk_ftn_thrd ;
	u32 	cbk_ftn_id;
	
	// using FIFO for internal queue
	int tail;
	int head;
	
	struct {
		u32  	obj_type;
		void 	*obj_ptr ;
		}entries[MSG_Q_SIZE] ;
		
} obj_queue_t;

#define NUM_CPUS 4
typedef struct __attribute__ ((__packed__)) cpu_threads_t{
	u8* 	stack_reset;
	u32 	cpu_id;
	thrd_t 	*current_thread;
	thrd_t 	*next_thread;
	u32   	num_assigned_threads;
	thrd_t 	*threads[8];
	
}cpu_threads_t;


// variables defined in linker script
extern char _firmware_start_;
extern char _firmware_end_ ;
extern char _stack_end_ ;
extern char _heap_start_;

void make_mmu_ttb(unsigned int cpu_num , unsigned int * ttb_base_address);
void halt_sec_cpus(unsigned int cpuID);
void release_sec_cpus();

void init_firmware_ftn(unsigned int cpuID ); 
void firmware_entry (unsigned int cpuID );
void core_loop_1_thrds ( uint32_t thrd1  );
void cpu_scheduler(cpu_threads_t * cpu_thrds , u8* stack_reset_ptr );
void cpu_thread_scheduler ( cpu_threads_t * cpu_thrds );
void run_callback_ftn(u32 arg_0, u32 arg_1, void* ftn_ptr, u8* thrd_stack_ptr );
void asm_save_context(cpu_threads_t* cpu_threads, u32* saved_stack_reg_addr );
void asm_run_thread(cpu_threads_t *cpu_threads, thrd_t *t, u8* stack_start);
void asm_restore_saved_thread(u32 saved_stack_reg);
void init_req_obj();

  




