 

#define MAX_ISP_THREADS 4
#define thrd_id_t int

#define MSG_IN_EXECUTE_FTN 		0x1

#define MSG_OUT_FTN_EXIT  		0x01
#define MSG_OUT_READ_HOST 		0x02
#define MSG_OUT_WRITE_HOST 		0x04
#define MSG_OUT_GET_DTX_REQ  	0x05
#define MSG_OUT_SYNC_NAND_OP   	0x07


typedef NvmeISPSCmd NvmeISP_CTRL_SCmd ;
typedef NvmeISPSCmd NvmeISP_DATATX_SCmd ;


#define DTX_STATUS_REQ_READY 1
#define DTX_STATUS_DMA_PROCESSING 2
#define DTX_STATUS_DMA_COMPLETE 3

/*
typedef struct {
	u32 	start_mem_page;
	u32 	size ;
	u32 	tx_type;
	u32 	request_code;
	void 	*thrd;
	isp_req_t *req ;
	u32 	status ;

} dtx_req_t;

*/


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

	//isp_req_t 	*exec_cmd_req;        
	isp_req_t* 		ctrl_cmd_req;
	//isp_req_t 	*submitted_ctrl_req;
	//bool 		ctrl_msg_isready ;
	//u32 		ctrl_cmd_msg ;


	//u32 		dtx_count;
	//dtx_req_t 	dtx[1] ;

	isp_nand_op_t * sync_nand_op_req ;
	
	//isp_host_dma_t* free_dma_req ;  		

	#define 		PENDING_DMA_QUEUE_SIZE 	8 
	void* 			pending_dma_req_queue_items [PENDING_DMA_QUEUE_SIZE] ;
	queue_t 		pending_dma_req_queue ;
	queue_t*		pending_dma ;

	void* 			completed_dma_req_queue_items [PENDING_DMA_QUEUE_SIZE] ;
	queue_t 		completed_dma_req_queue ;
	queue_t*		cmplt_dma ;

	isp_host_dma_t*	active_dma_req[PENDING_DMA_QUEUE_SIZE];

	u32  function_rtn;
	bool function_exited ;

}isp_ftn_thread_t;

 
u8* 	external_code_buffer ;
u32 	external_code_size;
u8* 	nand_op_buffer;
isp_applet_header_t * header ;
isp_ftn_thread_t threads[MAX_ISP_THREADS];

void msg_cbk (void* arg_0, u32 arg_1 );
void thrd_cbk (void* arg_0, u32 arg_1 );

#define SET_CBK_MSG(t,msg) ((void* )( 0 | ( t->index << 8 ) | msg ))
#define GET_CBK_MSG(arg_0) ( ((u32)arg_0) & 0xFF )

static inline isp_ftn_thread_t* GET_MSG_CBK_THRD(void* arg_0){
	u32 index = (((u32)arg_0) & 0xFF00) >> 8  ;
	if (index > 0 ){ VPRINTF(" ERROR @ %s %u " , __FUNCTION__ , __LINE__ ); }
	return &threads[index];
}


int 	recycle_ctrl_cmd_ftn_id ;
void recycle_control_command( u32 arg );


int init_isp_driver(){
	
	// set the request callback function
	int msg_cbk_ftn;
	msg_cbk_ftn = sys_add_thread_function( NVME_DRIVER_THREAD , msg_cbk );

	recycle_ctrl_cmd_ftn_id = sys_add_thread_function( NVME_DRIVER_THREAD , recycle_control_command);
	
	
	for (u32 i = 0 ; i < MAX_ISP_THREADS ; i++){
		threads[i].index = i ;
		threads[i].ctrl_cmd_req = NULL ;
		
		threads[i].thrd_id = ISP_THREAD_START_INDEX + i ;
		threads[i].isp_thrd_id = i + 1 ;
		threads[i].msg_in_q = ISP_Q_START_INDEX + (i * 2) ;
		threads[i].msg_out_q = ISP_Q_START_INDEX + (i * 2)+1 ;
		threads[i].applet_id = 0 ;;
		threads[i].ftn_id = 0 ;
		threads[i].arg_size = 0 ;
		threads[i].function_exited = true  ;
		
		threads[i].sync_nand_op_req = NULL ;

		//threads[i].free_dma_req = NULL ;
		
		#define INIT_CIRCULAR_QUEUE(queue_name , item_size ) \
		queue_name .head = 0 ; \
		queue_name .tail = 0 ; \
		queue_name .size = (int)item_size ; \
		queue_name .objs = &  ( CONCAT_MACRO(queue_name,_items) [0] ) ;


		INIT_CIRCULAR_QUEUE (threads[i].pending_dma_req_queue , PENDING_DMA_QUEUE_SIZE)
		INIT_CIRCULAR_QUEUE (threads[i].completed_dma_req_queue , PENDING_DMA_QUEUE_SIZE)
		threads[i].pending_dma = &(threads[i].pending_dma_req_queue) ;
		threads[i].cmplt_dma = &(threads[i].completed_dma_req_queue) ;


		//sys_create_thread(active_ftns[i].thrd_id);
		//threads[i].cbk_ftn = sys_add_thread_function( threads[i].thrd_id , thrd_cbk );
		// msg to thread queue
		//sys_create_obj_queue(threads[i].msg_in_q , NVME_DRIVER_THREAD , threads[i].cbk_ftn );
		// msg from thread queue
		//sys_create_obj_queue(Ithreads[i].msg_out_q , threads[i].thrd_id , msg_cbk_ftn );
	
		threads[i].arg_buffer = 0; // get_uncacheable_ram( MDT_SIZE );
	}
	
	// one thread for now
	threads[0].arg_buffer = get_uncacheable_ram( MDT_SIZE );
	sys_create_thread(threads[0].thrd_id);
	threads[0].cbk_ftn = sys_add_thread_function( threads[0].thrd_id , thrd_cbk );
	sys_create_obj_queue(threads[0].msg_in_q , NVME_DRIVER_THREAD , threads[0].cbk_ftn );
	sys_create_obj_queue(threads[0].msg_out_q , threads[0].thrd_id , msg_cbk_ftn );

	
	nand_op_buffer = get_uncacheable_ram( (1 << 24)); // 1MB // TODO, to be managed
	
	VPRINTF_CONFIG ( "external_code_buffer addr=0x%x , size=%uMB" , external_code_buffer, ISP_MAX_APPLET_BIN_SIZE >> 20 );

	return 0;
}


static inline void execute_command (NvmeISPSCmd *scmd, isp_req_t *req){
	
	isp_ftn_thread_t * free_t;
	
	// find a free thread
	free_t = &threads[0];
	free_t->applet_id = scmd->applet_id  ;
	free_t->ftn_id = scmd->ftn_id;
	free_t->arg_size = scmd->agument_size ;
	free_t->function_exited = false ;
	

	if( free_t->ctrl_cmd_req ){ 
		VPRINTF_LINE 
		free_t->ctrl_cmd_req->req_stage = REQ_COMPLETED ;
		sys_free_req_obj(free_t->ctrl_cmd_req);
	}
	free_t->ctrl_cmd_req = NULL ;
	
	//if (free_t->free_dma_req){ 
	//	free_t->free_dma_req->req_stage = REQ_COMPLETED ;  
	//	sys_free_req_obj(free_t->free_dma_req) ; 
	//}


	isp_host_dma_t * dma_req ;
	while ( ( dma_req = deQueue(free_t->cmplt_dma) ) ){
		VPRINTF_LINE
		dma_req->req_stage = REQ_COMPLETED ;
		sys_free_req_obj(dma_req);
	}

	for (u32 i = 0 ; i < PENDING_DMA_QUEUE_SIZE ; i++){
		dma_req = sys_malloc_req_obj(ISP_HOST_DMA_REQUEST);
		VPRINTF_LINE
		if ((enQueue (free_t->cmplt_dma , dma_req )) == NULL){
			// could not add to queue , free the obj 
			dma_req->req_stage = REQ_COMPLETED ;
			sys_free_req_obj(dma_req) ;VPRINTF_LINE
		}else {
			VPRINTF_LINE

		}
	}


	if (free_t->sync_nand_op_req){
		free_t->sync_nand_op_req->req_stage = REQ_COMPLETED ;
		sys_free_req_obj( free_t->sync_nand_op_req);
	}
	free_t->sync_nand_op_req = (isp_nand_op_t*) sys_malloc_req_obj(ISP_READ_PAGES_REQUEST ); 
		
	
	/*
	VPRINTF_ISP_IO("ISP IO: req:%u, sqid:%u, cqid:%u, \
						sqe:%u, cid:%u, nsid:%u, applet:%u ftn:%u{%s} \
						nHMBP:%u nAPiHMB:%u, prp1 {0x%x,0x%x}, prp2 {0x%x,0x%x} ",
						req->index ,
						(unsigned int)req->sqid ,
						(unsigned int)req->cqid,
						(unsigned int)req->sqe,
						(unsigned int)req->scmd.cid ,
						(unsigned int)req->scmd.nsid ,
						(unsigned int)req->applet_id,
						(unsigned int)req->ftn_id,"--", 0 , agument_size ,
						//(unsigned int)req->nHMBP ,
						//(unsigned int)req->nAPiHMB,
						(unsigned int)((reg64)req->scmd.prp1).Dwords.hbits,
						(unsigned int)((reg64)req->scmd.prp1).Dwords.lbits,
						(unsigned int)((reg64)req->scmd.prp2).Dwords.hbits,
						(unsigned int)((reg64)req->scmd.prp2).Dwords.lbits);
						*/
	
	
	if (free_t->arg_size > 24 ){
		// copy argument from the host buffer to ssd ram
		req->dma.data_size =  free_t->arg_size ; //req->nAPiHMB * hostMemPageSize;
		decode_prp(req->scmd.prp1 , req->scmd.prp2, &req->dma);
		req->dma.buffer_addr = free_t->arg_buffer;
		req->dma.c_op_smem_pg = 0;
		req->dma.c_op_len = free_t->arg_size ;//req->nAPiHMB * hostMemPageSize;
		sync_host_dma(&req->dma, PCI_DMA_READ,0,0);

	}else{
		// copy argument from the submission command 
		scmd_arg_t 	*scmdarg = (scmd_arg_t*)free_t->arg_buffer ;
		scmdarg->cdw10 = scmd->scmdarg.cdw10;
		scmdarg->cdw11 = scmd->scmdarg.cdw12;
		scmdarg->cdw12 = scmd->scmdarg.cdw12;
		scmdarg->cdw13 = scmd->scmdarg.cdw13;
		scmdarg->cdw14 = scmd->scmdarg.cdw14;
		scmdarg->cdw15 = scmd->scmdarg.cdw15;
	}
	
	sys_push_obj_to_queue(free_t->msg_in_q , free_t , MSG_IN_EXECUTE_FTN );
		
	req->ccmd_dw0.raw = free_t->isp_thrd_id;
	enqueue_io_ccmd (req->cqid, (io_req_common_t*)req); 
}


void post_dma_msg( isp_ftn_thread_t* t , isp_host_dma_t* dma_req , isp_req_t* ctrl_cmd_req ){

	//u32 iterator ;

	ctrl_cmd_req->ccmd_dw0.raw = dma_req->request_code ;

	dma_req->ctrl_cmd_index = ctrl_cmd_req->ctrl_cmd_index ;



	// place the dma_req in a list
	//for ( iterator = 0 ; iterator < PENDING_DMA_QUEUE_SIZE ; iterator++){
	//	if ( t->active_dma_req[iterator] == NULL ) { break ; }
	//}

	//if ( iterator >= PENDING_DMA_QUEUE_SIZE ){
	//	VPRINTF_ERROR ("  *** %s:%u *** " , __FUNCTION__ , __LINE__ );
	//}

	t->active_dma_req[0] = dma_req ;

	VPRINTF_ISP_IO ( " Posting DMA Msg: DMA Req:%u, start_mem_page:%u, size:%u, request_code:0x%x, ssd_addr:0x%x " , 
					dma_req->index , dma_req->start_mem_page , dma_req->size  , dma_req->request_code , dma_req->ssd_addr); 
	
				 
	// post control command completion
	enqueue_io_ccmd (ctrl_cmd_req->cqid, (io_req_common_t*)ctrl_cmd_req );

}

// work-around for preventing host OS from aborting control command when applet is running for a long time 
void recycle_control_command( u32 arg ){

	isp_ftn_thread_t *t = ( isp_ftn_thread_t* ) arg  ; 
	isp_host_dma_t* dma_req ;

	if (t->ctrl_cmd_req){

		// check if there pending msg exist
		if ( (dma_req = deQueue(t->pending_dma))){
			VPRINTF_LINE
			VPRINTF_ISP_IO ( " %s: pending dma req : Req:{%u request code:0x%x" , __FUNCTION__, dma_req->index , dma_req->request_code ); 	
			post_dma_msg(t , dma_req , t->ctrl_cmd_req );
			t->ctrl_cmd_req = NULL ;

		}else if(t->function_exited){
			VPRINTF_LINE
			// post control command completion 
			t->ctrl_cmd_req->ccmd_dw0.raw = 0 ; VPRINTF_LINE
			enqueue_io_ccmd (t->ctrl_cmd_req->cqid, (io_req_common_t*) t->ctrl_cmd_req);
			t->ctrl_cmd_req = NULL ;
		}
		else {
			//VPRINTF_ISP_IO ( "Recycling Control Command Req : %u " , t->ctrl_cmd_req->index );
			t->ctrl_cmd_req->ccmd_dw0.raw = ISP_APPLET_ABR_MSG_RC ;
			enqueue_io_ccmd (t->ctrl_cmd_req->cqid, (io_req_common_t*) t->ctrl_cmd_req);
			t->ctrl_cmd_req = NULL ;
		}
	} 
}

void control_command (NvmeISP_CTRL_SCmd *scmd, isp_req_t* req){

	VPRINTF_LINE
	u32 isp_thread_id = 0;
	isp_host_dma_t * dma_req ;

	// find the executing thread
	isp_ftn_thread_t *t = &threads[isp_thread_id];
	// TODO , check if thread is still running


	if (t->ctrl_cmd_req){
		VPRINTF_ERROR_LINE ("Multiple Control Command Received");
	}

	req->ctrl_cmd_index = scmd->scmdarg.cdw10 ;

	// check for pending dma_req 
	if(t->function_exited){VPRINTF_LINE
		// post control command completion 
		req->ccmd_dw0.raw = 0 ; VPRINTF_LINE
		enqueue_io_ccmd (req->cqid, (io_req_common_t*) req);

	}else if ( (dma_req = deQueue(t->pending_dma))){VPRINTF_LINE
		VPRINTF_ISP_IO ( " %s: pending dma req : Req:{%u request code:0x%x" , __FUNCTION__, dma_req->index , dma_req->request_code ); 	
		post_dma_msg(t , dma_req , req );
	}else {VPRINTF_LINE
		
		t->ctrl_cmd_req = req ;

		//VPRINTF_ISP_IO ( " Control Command Received Req : %u " , t->ctrl_cmd_req->index );
		
		sys_add_callback_ftn(recycle_ctrl_cmd_ftn_id, (u32)t );
	}

}


void data_transfer_command (NvmeISP_DATATX_SCmd* scmd, isp_req_t* req){

	u32 isp_thread_id = 0;
	isp_host_dma_t * dma_req ;
	dma_t *dma ;
	
	// find the executing thread
	isp_ftn_thread_t *t = &threads[isp_thread_id];
	// TODO , check if thread is still running


	VPRINTF_ISP_IO ( " TX Scmd : start mem page:%u , size :%u " , scmd->scmdarg.cdw10 , scmd->scmdarg.cdw11 );


	// find the dma_req 
	//u32 iterator ;
	dma_req = t->active_dma_req[0] ;
	//for (iterator = 0 ; iterator < PENDING_DMA_QUEUE_SIZE ; iterator++){
	//	if ( t->active_dma_req[iterator] != NULL ) { 
	//		 dma_req = t->active_dma_req[iterator] ;
	//		 break ; 
	//	}
	//}
	//// VPRINTF_ISP_IO ( " %s:%u  dma_req: %u size:%u " , __FUNCTION__ , __LINE__ ,  dma_req->index , dma_req->size  ) ;

	VPRINTF_ISP_IO ( " Transfering Data: DMA Req:%u, start_mem_page:%u, size:%u, request_code:0x%x, ssd_addr:0x%x" , 
					dma_req->index , dma_req->start_mem_page , dma_req->size , dma_req->request_code , dma_req->ssd_addr); 

	VPRINTF_ISP_IO ( " Current Tx : start_mem_page:%u,  size:%u  " , 
				    scmd->scmdarg.cdw10 , scmd->scmdarg.cdw11  ); 

	dma = & req->dma ;
	
	dma->data_size =  scmd->scmdarg.cdw11  ;//* hostMemPageSize ;
	decode_prp(scmd->prp1 , scmd->prp2, dma);
	dma->buffer_addr = dma_req->ssd_addr ;// + (scmd->scmdarg.cdw10 * hostMemPageSize)  ;
	dma->c_op_smem_pg = 0 ; 
	dma->c_op_len = dma->data_size ;  
	
	dma_req->req_stage = ISP_HOST_DMA_REQ_TX_ON ;

	//dma_dtx->status = DTX_STATUS_DMA_PROCESSING ;
	if 		(dma_req->tx_type == ISP_APPLET_READ_DTX)	sync_host_dma(dma,PCI_DMA_READ , 0, 0);
	else if (dma_req->tx_type == ISP_APPLET_WRITE_DTX)	sync_host_dma(dma,PCI_DMA_WRITE, 0, 0);

	dma_req->transfered_size += dma->data_size ;

	enqueue_io_ccmd (req->cqid , (io_req_common_t*)req );

	if ( dma_req->transfered_size == dma_req->size ){
		// transfer complete 
		dma_req->req_stage = ISP_HOST_DMA_REQ_CMPT ;
	}else {
		VPRINTF_ERROR ( " *** Error @%s:%u , dma_req:%u, transfered_size:%u , size:%u  *** " , __FUNCTION__ , __LINE__ ,
						dma_req->index , dma_req->transfered_size , dma_req->size  ) ;
		dma_req->req_stage = ISP_HOST_DMA_REQ_CMPT ; // TODO :: should not be here
	}

	//t->active_dma_req[iterator] = NULL ;
}

void isp_io (NvmeIOSCmd *scmd, isp_req_t *req){
	
	switch (scmd->opcode) {
		case ISP_IO_OPCODE:
			req->req_stage = ISP_EXEC_PENDING_POST ;
			execute_command ((NvmeISPSCmd*)scmd, req);
			break;

		case ISP_CTRL_OPCODE:
			req->req_stage = ISP_CTRL_PENDING_MSG ;
			control_command ((NvmeISP_CTRL_SCmd*)scmd,req);
			break;
			
		case ISP_DATA_TX_OPCODE:
			req->req_stage = ISP_DATA_TRANSFERING ;
			data_transfer_command ((NvmeISP_DATATX_SCmd*)scmd,req);
			break;

		default:
			break;
	}
}

u32 do_sync_nand_op(lpn_t start_lpn , u32 num_of_lpn  , u8* ssd_addr , u32 type ){
	
	isp_ftn_thread_t *t ;

	// detect the thread 
	t = &threads[0];

	t->sync_nand_op_req->type = type ;
	t->sync_nand_op_req->start_lpn = start_lpn;
	t->sync_nand_op_req->num_of_lpn = num_of_lpn;
	t->sync_nand_op_req->ssd_addr = ssd_addr;
	t->sync_nand_op_req->cbk_ftn_id = SYNC_CALL;
	t->sync_nand_op_req->status = 0 ;
	t->sync_nand_op_req->exec_thrd = t ;
		
	VPRINTF_LINE 

	t->sync_nand_op_req->req_stage = ISP_NAND_OP_INIT ;

	sys_push_obj_to_queue(t->msg_out_q, t->sync_nand_op_req , type );


	while (t->sync_nand_op_req->req_stage != ISP_NAND_OP_CMPLT ) {
		sys_yield_cpu();
	}
	
	return 0;
}


u32 _do_sync_host_dma_ (u32 start_mem_page, u8* ssd_addr, u32 size , u32 tx_type ){

	isp_ftn_thread_t *t ;
	isp_host_dma_t *dma_req ;
	volatile u32 return_tx_size ;

	// detect the thread 
	t = &threads[0];


	// get a dma req object
	if ( (dma_req = deQueue(t->cmplt_dma )) == NULL){
		VPRINTF_ERROR_LINE ( "" );
	}else{
		//VPRINTF_ISP_IO (" Using DMA Req Obj : %u " , dma_req->index );
	}  


	if ( (start_mem_page > CTRL_MSG_DTX_SPAGE_MAX) || ( size > CTRL_MSG_DTX_SIZE_MAX ) ){
		VPRINTF_ERROR ( " *** start_mem_page or size is larger than msg field start_mem_page:%u (MAX:%u) size:%u (MAX:%u)  *** " , 
			start_mem_page , CTRL_MSG_DTX_SPAGE_MAX , size , CTRL_MSG_DTX_SIZE_MAX );
	}


	dma_req->ssd_addr = ssd_addr;
	dma_req->start_mem_page = start_mem_page ;
	dma_req->size  = size ;	
	dma_req->exec_thrd = t ;
	dma_req->tx_type = tx_type ;
	dma_req->request_code = ( size << (CTRL_MSG_DTX_SPAGE_BITS + 2 ) ) | ( start_mem_page << 2 ) | tx_type ;
	dma_req->transfered_size = 0 ;
	dma_req->req_stage = ISP_HOST_DMA_REQ_INIT ;

	u32 num_host_pages = size / hostMemPageSize; if ( (size % hostMemPageSize )) num_host_pages ++;
	VPRINTF_ISP_IO ( " Do DMA operation: DMA Req:%u , start_mem_page:%u,  size:%u, num_host_pages:%u request_code:0x%x, ssd_addr:0x%x " , 
					dma_req->index , dma_req->start_mem_page , dma_req->size , num_host_pages , dma_req->request_code , dma_req->ssd_addr ); 
	

	if( ! enQueue(t->pending_dma , dma_req )) {
		VPRINTF_ERROR_LINE ("Pending DMA Request Queue is Full");
	} else {
		VPRINTF (" DMA Request Added to Queue Req:%u " , dma_req->index ); 
	}

	sys_push_obj_to_queue(t->msg_out_q, dma_req , ISP_HOST_DMA_REQUEST );


	while (1){
		if (dma_req->req_stage == ISP_HOST_DMA_REQ_CMPT ){
			VPRINTF_LINE
			break;
		}
		sys_yield_cpu();
	}
		VPRINTF_LINE
	
	return_tx_size = dma_req->transfered_size ;

	if (! ( enQueue(t->cmplt_dma , dma_req ) ) ){
		VPRINTF_ERROR_LINE ( "" );
	} 
	
	VPRINTF_ISP_IO ( " DMA operation Complete: DMA Req:%u , start_mem_page:%u,  size:%u, num_host_pages:%u request_code:0x%x, ssd_addr:0x%x " , 
					dma_req->index , dma_req->start_mem_page , dma_req->size , num_host_pages , dma_req->request_code , dma_req->ssd_addr ); 
	
	return return_tx_size ; 
}

u32 do_sync_host_dma(u32 start_mem_page, u8* ssd_addr, u32 size , u32 tx_type ){

	u32 return_tx_size;  
	u32 num_host_dma_op;
	u32 remaining_tx_size  ;
	u8* next_ssd_addr  ;
	u32 next_start_page ;
	u32 next_tx_size ;


	return_tx_size = 0 ; 
	remaining_tx_size = size ;
	next_ssd_addr = ssd_addr ;
	next_start_page = start_mem_page;
	next_tx_size = 0 ;

	num_host_dma_op = size / MAX_TX_SIZE;
	if ( (size % MAX_TX_SIZE )) num_host_dma_op ++;

	for ( u32 i = 0 ; i < num_host_dma_op ; i++){

		 
		next_tx_size = MIN( MAX_TX_SIZE , remaining_tx_size );
		VPRINTF_LINE
		return_tx_size += _do_sync_host_dma_(next_start_page , next_ssd_addr, next_tx_size , tx_type );
		VPRINTF_LINE
		remaining_tx_size -= next_tx_size   ;	
		next_ssd_addr += next_tx_size ;
		next_start_page +=  (next_tx_size / hostMemPageSize) ;
	}

	VPRINTF_LINE
	return return_tx_size ;
}



u32 do_read_flash_pages(lpn_t start_lpn , u32 num_of_lpn  , u8* ssd_addr , int call_back_ftn_id){	
	if ( call_back_ftn_id == SYNC_CALL){
		return do_sync_nand_op(start_lpn,num_of_lpn,ssd_addr,ISP_READ_PAGES_REQUEST);
	}
	return 0;
}

u32 do_program_flash_pages(lpn_t start_lpn , u32 num_of_lpn  , u8* ssd_addr , int call_back_ftn_id ){
	if ( call_back_ftn_id == SYNC_CALL){
		return do_sync_nand_op(start_lpn,num_of_lpn,ssd_addr,ISP_PROGRAM_PAGES_REQUEST);
	}
	return 0;
}

u32 do_read_host_buffer(u32 start_mem_page, u32 size, u8* ssd_addr, int call_back_ftn_id){
	if (call_back_ftn_id == SYNC_CALL ) {
		do_sync_host_dma(start_mem_page, ssd_addr, size , ISP_APPLET_READ_DTX ); 
	}
	return 0;
}

u32 do_write_host_buffer(u32 start_mem_page, u32 size, u8* ssd_addr, int call_back_ftn_id){
	if (call_back_ftn_id == SYNC_CALL ) {
		do_sync_host_dma(start_mem_page, ssd_addr, size , ISP_APPLET_WRITE_DTX ) ;
	}
	return 0;
}



u32 current_ofs = 0;
u8* alloc_page_buffer(u32 num_of_lpn){
	u8* ptr ;
	if ((current_ofs + (num_of_lpn << HOST_DS)) > (1<<24)){
		VPRINTF_ERROR ( " ** ISP %s error : current_offset:%u, num_of_lpn:%u " ,
					   __FUNCTION__ , current_ofs , num_of_lpn  );
		return 0;
	}
	ptr = nand_op_buffer + current_ofs;
	current_ofs += (num_of_lpn << HOST_DS) ;
	VPRINTF_ERROR ( " ISP %s : current_offset:%u, num_of_lpn:%u " ,
				   __FUNCTION__ , current_ofs , num_of_lpn  );
	
	return ptr ;
}


void free_all_page_buffer(){
	current_ofs = 0;
}

 
int add_applet( void * applet_ptr , unsigned int size ){
	
	unsigned int applet_id = 0;
	
	header = (isp_applet_header_t *) applet_ptr ;
	
	// check the magic string 
	unsigned char magic_string[16] = "snp isp applet"  ;
	uint32_t mis_match = 0 ;
	for (int ix = 0 ; ix < 8 ; ix++){
		if ( magic_string[0] != header->magic_string[0] ){
			VPRINTF_CONFIG (" %u error " , ix ) ;
			mis_match++ ;
		}
	}

	if(mis_match)return 5 ;
	
	VPRINTF_CONFIG ("ISP Applet:  Name:%s , No. global functions:%u , size:%u , init function address:0x%x " ,
					header->name  , 
					(unsigned int) header->num_ftn , 
					(unsigned int) (header->applet_end - header->applet_start),
					(unsigned int) header->initialize_ftn );
	
	for (u32 i = 0 ; i < header->num_ftn ; i++ ) {
		VPRINTF_CONFIG( " Applet global function: Index:%u Name:%s , Addr:0x%x" ,
					   i+1 , 
					   header->functions[i].name ,
					   header->functions[i].entry_address  );
	}
	
	// call the applet initialization function
	uint32_t (*initialize_ftn)(void * ) =  (uint32_t (*)(void * ))header->initialize_ftn;
	
	syscall_meta_t syscall_meta;
	syscall_meta.read_host_buffer = &do_read_host_buffer;
	syscall_meta.write_host_buffer = &do_write_host_buffer;
	syscall_meta.read_flash_pages = &do_read_flash_pages;
	syscall_meta.program_flash_pages = &do_program_flash_pages;
	syscall_meta.alloc_page_buffer = &alloc_page_buffer;

	syscall_meta.print_string = &_sync_print_ ;	 
	syscall_meta.simple_snprintf = &snprintf_;
	syscall_meta.simple_strcat = &simple_strcat;
	syscall_meta.get_print_buffer = &get_buff ;
	syscall_meta.queue_print = &queue_print;


	uint32_t err = initialize_ftn( &syscall_meta ) ; 

	if (err) {
		VPRINTF_CONFIG ( "initialization error :%u " , err )
		return 6 ;
	}
	
	// store applet
	return applet_id ;
}


void thrd_cbk (void* arg_0 , uint32_t arg_1 ){
	
	VPRINTF_LINE

	if (arg_1 == MSG_IN_EXECUTE_FTN){
	
		isp_ftn_thread_t* t ;
		isp_io_ftn_t ftn_ptr;
	
		t = (isp_ftn_thread_t*)arg_0;
		ftn_ptr = (isp_io_ftn_t)header->functions[t->ftn_id-1].entry_address;
	
		VPRINTF_ISP_IO (" ISP Function Call: ftn_index%u @addr:0x%x ", t->ftn_id , ftn_ptr );
		t->function_rtn = ftn_ptr (t->arg_buffer , t->arg_size  );
		VPRINTF_ISP_IO (" ISP Function Exit: return value:%u ",  t->function_rtn );
	 	free_all_page_buffer();
		t->function_exited = true ;
		sys_push_obj_to_queue(t->msg_out_q, t , ISP_EXECUTION_EXIT ) ;
	}
}


void msg_cbk (void* obj , u32 req_type ){
	
	VPRINTF_LINE	

	isp_ftn_thread_t* t ;
	isp_host_dma_t* dma_req ;
	  
	switch (req_type){

		case ISP_HOST_DMA_REQUEST :
			dma_req = (isp_host_dma_t*)obj ;
			t = dma_req->exec_thrd ; 

			// check if control command is available
			if(t->ctrl_cmd_req){
				VPRINTF_LINE

				post_dma_msg(t , dma_req , t->ctrl_cmd_req  ) ;	
				t->ctrl_cmd_req = NULL;
			
			}else{
				VPRINTF_LINE
			
				//if( ! enQueue(t->pending_dma , dma_req )) {
				//	VPRINTF_ERROR_LINE ("Pending DMA Request Queue is Full");
				//} else {
				//	VPRINTF_LINE 
				//}
			}



			break ;

		case ISP_READ_PAGES_REQUEST :
		case ISP_PROGRAM_PAGES_REQUEST :
			sys_push_to_ftl((void*)obj , req_type ); VPRINTF_LINE
			break ;

		case ISP_EXECUTION_EXIT :
			t = (isp_ftn_thread_t* )obj ;
			
			
			//VPRINTF_CONFIG(" %s: ISP Function Exit: return value:%d ", __FUNCTION__ , t->function_rtn );
		
			//free resources
			//sys_lpg_buff_free_all();
			//sys_free_req_obj(req);

			if( t->ctrl_cmd_req  ){
				VPRINTF_LINE
				t->ctrl_cmd_req->ccmd_dw0.raw = 0  ;	VPRINTF_LINE
				enqueue_io_ccmd (t->ctrl_cmd_req->cqid, (io_req_common_t*) t->ctrl_cmd_req );
				t->ctrl_cmd_req = NULL ;
			}

			break ;	
		default:
			break;
	}

 
}


