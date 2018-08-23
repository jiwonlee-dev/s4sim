



#define MAX_ISP_THREADS 4
#define thrd_id_t int

#define MSG_IN_EXECUTE_FTN 		0x1

#define MSG_OUT_FTN_EXIT  		0x01
#define MSG_OUT_READ_HOST 		0x02
#define MSG_OUT_WRITE_HOST 		0x04
#define MSG_OUT_GET_DTX_REQ  	0x05
//#define MSG_OUT_READ_NAND   	0x06
//#define MSG_OUT_PROGRAM_NAND  0x07
#define MSG_OUT_SYNC_NAND_OP   	0x07


typedef NvmeISPSCmd NvmeISP_CTRL_SCmd ;
typedef NvmeISPSCmd NvmeISP_DATATX_SCmd ;
typedef NvmeISPSCmd NvmeISP_FILE_SCmd ;


#define DTX_STATUS_REQ_READY 1
#define DTX_STATUS_DMA_PROCESSING 2
#define DTX_STATUS_DMA_COMPLETE 3


typedef struct {
	u64			filesize ;
	u32 		num_of_lbas ;				// 0 indexed 
	u32 		num_of_lba_ranges ;			// 0 indexed 
	frange_t*	range ;
	
	bool 		lba_range_loaded ;
	u32 		next_range_index ; 			// range index where the last page index was located , 0 indexed
}host_file_t ;

typedef struct {
	u32 	start_mem_page;
	u32 	size ;
	u32 	tx_type;
	u32 	request_code;
	void 	*thrd;
	isp_req_t *req ;
	u32 	status ;

} dtx_req_t;


typedef struct {
	u32 		index ;
	u32 		isp_thrd_id ;  	// id sent to host
	thrd_id_t 	thrd_id ;
	
	app_id_t 	applet_id;
	ftn_id_t 	ftn_id;
	u32 		arg_size ;
	u8* 		arg_buffer;

	int 		cbk_ftn ;
	int 		msg_in_q;
	int 		msg_out_q;

	isp_req_t 	*req;       // check free location
	isp_req_t 	*ctrl_req;
	isp_req_t 	*submitted_ctrl_req;
	bool 		ctrl_data_pending;
	u32 		pending_ctrl_cdw0;


	u32 		dtx_count;
	dtx_req_t 	dtx[1] ;

	isp_nand_op_t * sync_nand_op_req ;
	

	u32  function_rtn;
	bool function_exited ;

}isp_ftn_thread_t;




extern u32 do_read_flash_pages(lpn_t start_lpn , u32 num_of_lpn  , u8* ssd_addr , int call_back_ftn_id);
extern u32 do_program_flash_pages(lpn_t start_lpn , u32 num_of_lpn  , u8* ssd_addr , int call_back_ftn_id );


extern void hfopen(SSDFile* pFile, uint64_t fileSize, uint64_t numRanges, lpn_range_t* pLpnRangeList);
extern void hfclose(SSDFile* pFile);
extern void hfseek(SSDFile* pFile, int64_t offset, int origin);
extern int hfread(SSDFile* pFile, char* dest, uint32_t size, uint32_t count);
extern int hfwrite(SSDFile* pFile, char* src, uint32_t size, uint32_t count);
extern int hfscanf(SSDFile* pFile, const char* format, ...);

#define SSDFile_open 	hfopen 
#define SSDFile_close 	hfclose 
#define SSDFile_fseek 	hfseek
#define SSDFile_fread 	hfread 
#define SSDFile_fwrite 	hfwrite 
#define SSDFile_fscanf 	hfscanf 





