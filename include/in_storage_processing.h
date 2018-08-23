
#include "issd_types.h"

#ifndef IN_STORAGE_PROCESSING 
#define IN_STORAGE_PROCESSING

#define START_PAGE_MASK   			0x0000FFFC
#define SIZE_MASK  		  			0xFFFF0000

/* In Storage Processing Declarations */
#define ISP_IO_OPCODE 				0x80
#define ISP_CTRL_OPCODE 			0x81
#define ISP_DATA_TX_OPCODE 			0x82
#define iSSD_MAX_PAGE_SIZE 			16384 
#define ISP_MAX_PAGE_RANGES 		64
#define ISP_MAX_APPLET_FUNCTIONS 	64
#define ISP_MAX_APPLET_BIN_SIZE 	0x10000000    /* 32 MB  */
#define ISP_MAX_LBA_RANGES_INFO		16 
#define ISP_ARG_IN_SCMD_SIZE		24


#define SYNC_CALL 					-1
#define ASYNC_CALL 					0

// Control Command Messages
#define ISP_APPLET_READ_DTX 		0x1
#define ISP_APPLET_WRITE_DTX 		0x3
#define DTX_BIT_MASK 				0x3
#define ISP_APPLET_ABR_MSG			0x2  		// message code for all other control command message 
#define ISP_APPLET_ABR_MSG_RC		0x102  		// message code to recycle control command 


#define CTRL_MSG_DTX_SPAGE_BITS   	12		
#define CTRL_MSG_DTX_SPAGE_MAX   	0xFFF 	
#define CTRL_MSG_DTX_SIZE			18
#define CTRL_MSG_DTX_SIZE_MAX		0x3FFFF
//2
//12
//18 


#define HOST_DMA_REQUEST_TIMEOUT	0x1
#define PROGRAM_PERMISSION_DENIED	0x2
#define READ_PERMISSION_DENIED		0x3
#define INVALID_PAGES				0x4


typedef struct __attribute__ ((__packed__)) ISP_LBA_Ranges_Information {
	u32 	type;
	u32 	num_of_ranges;
	u32 	size;
	u32 	filename_offset;
	u32 	ranges_offset ;
} isp_lr_t;

typedef struct __attribute__ ((__packed__))  ISP_LBA_Arguments {
	uint32_t num_of_lba_info;
	isp_lr_t lba_info[ISP_MAX_LBA_RANGES_INFO];
} isp_lba_arg_t ;

typedef struct __attribute__ ((__packed__))  ISP_LBA_Range_t{
	lpn_t 		start_lpn;
	uint32_t 	num_of_lpn;
	uint32_t	file_byte_offset;
	//uint32_t	page_offset;
}lpn_range_t;

typedef struct __attribute__ ((__packed__)) Argument_in_Nvme_SCMD {
	u32 	cdw10;
	u32 	cdw11;
	u32 	cdw12;
	u32 	cdw13;
	u32 	cdw14;
	u32 	cdw15;
}scmd_arg_t;


typedef struct _packed  NvmeVendor_InStrorageProcessingCommand{ 
	u32 	opcode			: 8; 
	u32 	fuse 		 	: 2; 
	u32 	applet_id 		: 5;
	u32 	p 				: 1;
	u32 	cid 			: 16;
	u32    	nsid ;		
	u8 		ftn_id ; 
	u8  	meta_data ;
	u16 	exec_timeout ;	
	u32 	agument_size;
	u64    	mptr ;		
	u64    	prp1 ;		
	u64    	prp2 ;	
	scmd_arg_t 	scmdarg ;
}NvmeISPSCmd ;


typedef u32 	(*isp_io_ftn_t) 		(u8*, u32 )  ;
typedef u32 	(*host_buffer_ftn_t) 	(u32 , u32 , u8* , int )  ;
typedef u32 	(*flash_access_ftn_t)	(lpn_t , u32  , u8* , int )  ;
typedef u8* 	(*alloc_page_buffer_ftn_t)(u32 );
typedef void 	(*print_string_ftn_t)	(char*);


typedef int 	(*simple_snprintf_t)	(char*, size_t  , char* , ...);
typedef char* 	(*simple_strcat_t)		(char* , const char* );
typedef char* 	(*get_print_buffer_t)	();
typedef void 	(*queue_print_t)		();


#ifdef __cplusplus
extern "C" {
#endif
	
	typedef uint8_t ftn_id_t ;
	typedef uint8_t app_id_t ;


	typedef struct  __attribute__ ((__packed__)) {
	unsigned char magic_string[16] ;
	char 		name [64];
	u32 	applet_start;
	u32    	applet_end ;
	u32   	initialize_ftn ;  			// called automatically after code uploaded 
	u32 	finalize_ftn;    			// called before firmware cleans applet from ssd memory
	u32 	num_ftn ;
	
	struct functions_t {
		uint32_t 		entry_address ;
		char 			name [64];
	}functions[ISP_MAX_APPLET_FUNCTIONS] ;
	
	}isp_applet_header_t ;


	typedef struct {

		// list of function pointers for 
		host_buffer_ftn_t 		read_host_buffer;
		host_buffer_ftn_t 		write_host_buffer;
		flash_access_ftn_t 		read_flash_pages;
		flash_access_ftn_t 		program_flash_pages;
		alloc_page_buffer_ftn_t alloc_page_buffer ;
		
		print_string_ftn_t 		print_string;
		simple_snprintf_t 		simple_snprintf;
		simple_strcat_t 		simple_strcat;
		get_print_buffer_t 		get_print_buffer;
		queue_print_t 			queue_print;

	}syscall_meta_t ;


#ifdef __cplusplus
}
#endif



#endif





