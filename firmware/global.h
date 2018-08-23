 

#ifndef GLOBAL_HEADER 
#define GLOBAL_HEADER

#include <stdint.h>
#include <stdio.h>
#include "global_macro.h"
 
#include <issd_types.h>
#include "../include/nvmeDataStructures.h"
#include "../include/iSSDHostIntCmdReg.h"
#include "../include/iSSDNandCtrlCmdReg.h"
#include "../include/in_storage_processing.h"
#include "util.h"



typedef struct SSD_Configuration {
	// Namespace configuration 
	uint32_t 		host_ds ;
	uint32_t    	nsze;
	uint32_t    	ncap;
	uint32_t    	nuse;	
}ssd_config_t  ;


typedef struct { 
	uint32_t		index ;
	uint32_t		size ; 
	uint64_t		host_addr ;
} dma_entry_t  ;

typedef struct{
	dma_entry_t		entries[256]; 		// TODO , reduce this size
	uint64_t		data_size ;
	uint32_t		count ;
} dma_list_t ;

// metadata for dma operation for each page in the read/write io request
typedef struct dma_operation_t {
	u8*			ssd_addr;					// the SSD RAM pointer 
	u32			len;
	u64			host_p_addr;				// host physical address for host dma
	bool		host_dma_complete;
}dma_op_t ;

typedef struct host_dma_meta_t {
	u32			data_size;
	u32			n_ops ; 					// num of operations
	bool		host_dma_complete ; 		// true if all pages have been read/written to host
	u8*			buffer_addr;
	
	u32 		c_op_smem_pg; 				// current operation start memory page 
	u32 		c_op_len ;					// current operation transfer lenght
	
	dma_op_t 	list[MDTS_PAGES] ;
}dma_t; 


#define REQUEST_COMMON_FIELDS 	\
	u32			type ;			\
	u32 		index ; 		\
	u32 		req_stage; 

#define IO_REQUEST_COMMON_FIELDS \
	REQUEST_COMMON_FIELDS 		\
	u32			sqid ;			\
	u32			cqid ; 			\
	u32			sqe	;			\
/* completion command status */ \
	u16			status ; 		\
	u16			resv1 ;			\
	Dword 		ccmd_dw0 ;		\
/* copy of submission command */ \
	NvmeIOSCmd	scmd ;		


typedef struct {
	REQUEST_COMMON_FIELDS
}req_common_t ;

typedef struct {
	IO_REQUEST_COMMON_FIELDS
}io_req_common_t ;


typedef struct AdminRequest {
	REQUEST_COMMON_FIELDS
	NvmeSCmd		scmd ;
	uint32_t		scmd_entry;
	dma_t 			dma ;
	
	// completion command status 
	Dword			ccmd_cdw0 ;
	uint16_t		status ;
	
}AdminRequest;
typedef AdminRequest a_req_t;


typedef struct Read_IO_Request {
	
	IO_REQUEST_COMMON_FIELDS
	
	dma_t   	dma;

	// additional metadata for read io request
	bool 		nand_io_complete ; 			// true if all pages are available in ssd buffer

	struct {
		lpn_t	lpn;					// logical page number ( same as LBA)
		u32 	lba_offset_start;
		u32		lba_offset_end ;
		ppa_t	ppa;					// the physical page address (NAND page) after FTL translation
		bool	nand_io_complete;
	}read_page_meta[MDTS_PAGES];
	
}r_req_t ;

typedef struct Write_IO_Request {
	IO_REQUEST_COMMON_FIELDS
	dma_t   	dma;
}w_req_t;

typedef struct Flush_IO_Request {
	IO_REQUEST_COMMON_FIELDS
}f_req_t;

typedef struct DSM_IO_Request {
	IO_REQUEST_COMMON_FIELDS
	dma_t  		dma;
}dsm_req_t;

typedef struct ISP_IO_Request {
	IO_REQUEST_COMMON_FIELDS
	dma_t   	dma;
	u32 		ctrl_cmd_index;
}isp_req_t;


// FTL module expects only write back cache request object for write operation
typedef struct write_back_cache_request {
	REQUEST_COMMON_FIELDS 

	u32 		num_program_pgs	; 			// number of pages to program
	bool		nand_io_complete ; 			// true if all pages have been successfully programmed to flash memoey 
	
	struct wbc_page_meta_t{
		lpn_t		lpn  ;
		u8*			ssd_addr;
		uint16_t	valid ;
		uint16_t	nand_io_complete;
	} pages [WBC_PAGES];
	
} wbc_req_t ;


// request structure for reading logical pages from flash memory
typedef struct {
	REQUEST_COMMON_FIELDS
	uint32_t		status ;
	lpn_t 			start_lpn;
	uint32_t 		num_of_lpn;
	uint8_t*		ssd_addr ;
	ftn_t			cbk_ftn_id;

	void			*exec_thrd;
}isp_nand_op_t;

typedef struct ISP_HOST_DMA_REQUEST_t
{
	REQUEST_COMMON_FIELDS 

	u8*  			ssd_addr;
	u32 			start_mem_page ;
	u32 ddd[4];
	u32 			size ;
	u32 ddsd[4];
	
	u32 			tx_type;
	u32 			request_code ;
	void 			*exec_thrd;
	u32 			transfered_size ;
	u32 			ctrl_cmd_index ;

	u32 			status ;

} isp_host_dma_t ;


// Global Configuration of the SSD
extern ssd_config_t ssd_config ;



/* *********  Declaration of module interfaces ********** */


/*		Kernel Interface 		*/
extern int 	sys_create_thread (int thrd);
extern int 	sys_add_thread_function (int thrd , void* ftn_ptr);
extern int 	sys_create_obj_queue (int queue_id , int push_thrd , int cbk_ftn_id );
// TODO , use enqueue_obj  instead of push , and dequeue_obj instead of pop 
// sounds more nice 
extern int 	sys_push_obj_to_queue (int queue, void * obj_ptr, uint32_t obj_type );
extern int 	sys_add_callback_ftn (int ftn_id , uint32_t arg1 );
extern void sys_yield_cpu ();
extern void sys_push_to_ftl (void* req, uint32_t req_type);
extern void sys_push_to_nvme (void* req, uint32_t req_type);
extern void*sys_malloc_req_obj (u32 req_type);
extern io_req_common_t * sys_find_io_request( u32 sqid , u32 cid );
extern void * sys_get_active_req_objects();
extern void sys_free_req_obj (void * req_obj);
extern void sys_host_shutdown();

extern unsigned char * get_dev_address( int DeviceID );
extern unsigned int get_cpu_id();
extern unsigned char * get_uncacheable_ram(unsigned int size);
// UART Device printing   	
extern void printToDev(const char *s);
extern void printCharToDev(unsigned char  );



/*  	Host Direct Memory Access (DMA) Module Interface  */
#define Fragmented_buffer_support 0
int sync_read_scmd (u32 sqid, u32 sqe, u64 host_p_addr, u8* ssd_addr, u32 ncmds);
int sync_post_ccmd (u32 cqid, u32 cqe, u64 host_p_addr, u8* ssd_addr, u32 ncmds, u32 vector, u32 irq_enabled);
int sync_read_io_dma (r_req_t* req);
int sync_write_io_dma (w_req_t* req);
int sync_host_dma (dma_t* dma, u32 op, u32 start_mem_page, u32 len );
//int async_read_io_dma (r_req_t* req, ftn_t cbk_ftn);
//int async_write_io_dma (w_req_t* req, ftn_t cbk_ftn);
//u32 async_host_dma (u32 op, dma_list_t *list, u8* ssd_addr, u32 data_size);



/* 		Nvme Module Interface 		*/
extern u32 hostMemPageSize;
extern u32 hostPageBits ;
extern int completed_req_cbk_ftn_id;	
extern void enqueue_io_ccmd(uint32_t cqid , io_req_common_t *req );
extern int decode_prp(u64 prp1, u64 prp2, dma_t *dma_obj ) ;	
extern int shutdown_nvme_driver();



/* 	 	Page Buffer Manager Interface 	*/ 
int get_read_io_buffer (r_req_t *req );
int free_read_io_buffer(r_req_t *req);
int get_write_io_buffer (w_req_t *req );
int free_write_io_buffer(w_req_t *req);
unsigned char *get_page_buffer() ;
int free_page_buffer();
int flush_all_wbc() ;
void free_wbc_buffer(wbc_req_t * wbc);



/* 		FTL Module Interface 		*/
extern int shutdown_ftl_module(); 		
extern int process_req_cbk_ftn_id;			



/*    	NAND I/O Interface 		 */
void* begin_read_op(bool pg_cbk);
uint32_t add_read_op_page(void* rop , ppa_data_t ppa , unsigned char * ssd_addr , void * callback);
int sync_end_read_op(void* rop );
void* begin_program_op(bool pg_cbk);
uint32_t add_program_op_page(void* pop, ppa_data_t ppa , unsigned char * ssd_addr , void * callback);
int sync_end_program_op(void* pop );
int shutdown_nand_io_driver();



/* 		ISP Driver Interface 	*/
extern u8* external_code_buffer;
extern u32 external_code_size;
extern int add_applet( void * applet_ptr , unsigned int size );
extern void isp_io (NvmeIOSCmd *scmd, isp_req_t *req);



#endif




