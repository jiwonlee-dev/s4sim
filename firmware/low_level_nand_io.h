 

#ifndef LOW_LEVEL_NAND_IO_HEADER 
#define LOW_LEVEL_NAND_IO_HEADER


// low level nand io operations
struct read_flash_op_t { 			// read operation list
	uint32_t 		type ;		// type of operation : read , program , erase , etc
	uint32_t		index ;
	uint32_t		nop	;		// number of pages;
	bool				pg_cbk; 	// true if a callback should be called for each completed page read 
	void				*final_cbk; // function in FTL to call back after all page read operation is complete
	void 				*ftl_data;	// pointer set by FTL for FTL's internal purposes , eg. the request object from nvme
	
	struct read_page_t {
		ppa_t		ppa ;
		unsigned char*   ssd_addr ;
		void			*cbk;		// function in FTL to call back after this page is read from flash
		uint32_t	req_index;	// nvme request index associated with this nand page operation
	} pages[128];
	
} ;

struct program_flash_op_t { 		// program operation list
	uint32_t 		type ;		// type of operation : read , program , erase , etc
	uint32_t		index ;
	uint32_t		nop	;		// number of pages;
	bool			pg_cbk; 	// true if callback should be triggered for each completed page program 
	void			*final_cbk; // function in FTL to call back after all page program operation is complete
	void 			*ftl_data;	// pointer set by FTL for FTL's internal purposes , eg. can assing the original request object
	
	struct program_page_t {
		ppa_t			ppa ;
		unsigned char*	ssd_addr;
		void			*cbk;		// function in FTL to call back after this page is programmed to flash
	}pages[128];
	
} ;

typedef struct read_flash_op_t 		rop_t;
typedef struct program_flash_op_t		pop_t;

void initialize_nand_ctrl(uint32_t arg1);
void sync_nand_io_cbk(void* io , uint32_t io_type);
void init_op_obj();
void * malloc_op_obj(uint32_t req_type);
void free_op_obj(void * op_obj);


#endif
