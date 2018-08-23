 

#ifndef NVME_DRIVER_HEADER
#define NVME_DRIVER_HEADER

typedef struct _packed nvmescmd_cp {
	uint64_t 	cdw0_1 ;	 
	uint64_t    res1;		
	uint64_t    mptr;		
	uint64_t    prp1;		 
	uint64_t    prp2;		   
	uint64_t 	cdw10_11 ; 	 
	uint64_t 	cdw12_13 ;
	uint64_t 	cdw14_15 ;
} nvmescmd_cp;

/** Submission Queue meta data structure */
typedef struct NvmeSQueue {   
	uint16_t    sqid;
	uint16_t    cqid;
	uint32_t    head;			// SQ head position
	uint32_t    tail;			// SQ tail doorbell
	reg*		tailBDReg ;		// address for reading DB register from host controller
	uint32_t    size;
	uint64_t    hostAddr;		// SQ dma address	 
} NvmeSQueue;

/** Completion Queue meta data structure */
typedef struct NvmeCQueue { 
	uint8_t     phase;
	uint8_t		irq_enabled;
	uint16_t    cqid;
	uint32_t    head;			// CQ head doorbell
	reg*		headBDReg ;
	uint32_t    tail;			// CQ tail position
	uint32_t    vector;
	uint32_t    size;
	uint64_t    hostAddr;		// CQ dma address
	
	// pending post completion queue
	struct{
		int tail;
		int head;
		int size;
		bool full;
		bool empty;
		io_req_common_t* objs[INTERNAL_IO_REQ_QUEUE];
	}pending;
	
} NvmeCQueue;



typedef struct __attribute__ ((__packed__)) __ctrlReg 
{
	reg 	cap_lo;
	reg 	cap_hi;
	reg		vs;		 
	reg		intms;	
	reg		intmc;	
	reg		cc;
	reg		reserved1;
	reg		csts;
	reg		reserved2;
	reg 	aqa;
	reg64	asq;
	reg64	acq;
	reg		reserved3[0x3f2];
	reg		sq_tdbl;		
	reg		cq_hdbl;		
	
} hostCtrlReg    ;

void completed_req_cbk( void * req , uint32_t type );
void initialize_nvme(uint32_t arg_0);
void check_door_bell(uint32_t arg_0);
void post_admin_ccmds();
void post_io_ccmds(uint32_t cqid);


#endif
