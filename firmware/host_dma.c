 
#include "nvme_driver.h"
#include "host_dma.h"

#define SYNC_DMA_Q   10  
unsigned char	*host_int_base_reg;
hostCmdReg		*cmdReg ;
hostCtrlReg		*ctrlReg ;

reg* regData = 0;
reg	 regVar = 0;
sync_dma_t sync_dma ;

 
int init_host_dma(){
	
	int id = sys_add_thread_function(HOST_DMA_THREAD , sync_dma_cbk );
	sys_create_obj_queue(SYNC_DMA_Q , NVME_DRIVER_THREAD , id );

	return 0;
}


int sync_read_scmd(u32 sqid, u32 sqe, u64 host_p_addr, u8* ssd_addr, u32 ncmds){
	
	sync_dma.dma_type = PCI_DMA_SQ_READ ;
	sync_dma.status = 0;
	
	sync_dma.sqid = sqid ;
	sync_dma.sqe = sqe ;
	sync_dma.host_p_addr = host_p_addr ;
	sync_dma.ssd_addr = ssd_addr;
	sync_dma.num_of_entries = ncmds ;
	
	sys_push_obj_to_queue(SYNC_DMA_Q , &sync_dma , sync_dma.dma_type );
	
	while (1){
		if (sync_dma.status > 0 ) break ;
		sys_yield_cpu();
	}
	return 0 ;
}

int sync_post_ccmd(u32 cqid, u32 cqe, u64 host_p_addr , u8* ssd_addr, u32 ncmds, u32 vector, u32 irq_enabled){
	
	sync_dma.dma_type = PCI_DMA_CQ_WRITE ;
	sync_dma.status = 0;
	
	sync_dma.cqid = cqid ;
	sync_dma.cqe = cqe ;
	sync_dma.host_p_addr = host_p_addr ;
	sync_dma.ssd_addr = ssd_addr;
	sync_dma.num_of_entries = ncmds ;
	sync_dma.vector = vector ;
	sync_dma.irq_enabled = irq_enabled ;
	
	sys_push_obj_to_queue(SYNC_DMA_Q , &sync_dma , sync_dma.dma_type );
	
	while (1){
		if (sync_dma.status > 0 ) break ;
		sys_yield_cpu();
	}
	return 0;
}


int sync_read_io_dma(r_req_t * req){
	req->req_stage = REQ_STAGE_DMA ;
	req->dma.host_dma_complete = false ;
	sys_push_obj_to_queue(SYNC_DMA_Q , (void*)req , READ_IO_DMA_OPERATION );
	
	while (!req->dma.host_dma_complete){
		sys_yield_cpu();
	}
	return 0 ;
}

int sync_write_io_dma (w_req_t * req ){
	req->req_stage = REQ_STAGE_DMA ;
	req->dma.host_dma_complete = false ;
	sys_push_obj_to_queue(SYNC_DMA_Q , (void*)req , WRITE_IO_DMA_OPERATION);
	
	while (!req->dma.host_dma_complete){
		sys_yield_cpu();
	}
	return 0 ;
}

int sync_host_dma(dma_t* dma, u32 op, u32 start_mem_page, u32 len ){
	
	dma->host_dma_complete = false;
	sys_push_obj_to_queue(SYNC_DMA_Q , dma , op );
	
	while (!dma->host_dma_complete){
		sys_yield_cpu();
	}
	return 0;
}


int do_sync_read_scmd(){
	
	host_int_base_reg = get_dev_address(DEV_HOST_INT);
	cmdReg = (hostCmdReg *)host_int_base_reg ;
	
	reg64 dma = (reg64) sync_dma.host_p_addr ;
	
	setReg64( cmdReg->sqeDmaAddr , dma ) ;
	setReg  ( cmdReg->squeue , sync_dma.sqid ) ;
	setReg  ( cmdReg->sqe , sync_dma.sqe  ); 
	setReg  ( cmdReg->sqAddr , ((uint32_t)sync_dma.ssd_addr) ) ;
	setReg  ( cmdReg->cmd , PCI_DMA_SQ_READ ) ;
	
	// reading sq is synchronous , wait until host interface has finished copying command to ssd 
	reg cmdStatus =0;
	while (1){
		getReg( cmdReg->cmdStatus , cmdStatus)
		if (cmdStatus == COMPLETE  ) break ;
		sys_yield_cpu();
	}
	sync_dma.status = 1;
	return 0 ;
}

int do_sync_post_ccmd(){
	
	host_int_base_reg = get_dev_address(DEV_HOST_INT);
	cmdReg = (hostCmdReg *)host_int_base_reg ;
	
	reg64 dma = (reg64)sync_dma.host_p_addr ; 
	
	setReg (cmdReg->cqueue , sync_dma.cqid) ;
	setReg (cmdReg->cqe , sync_dma.cqe );
	setReg (cmdReg->cqAddr , sync_dma.ssd_addr );
	setReg64 (cmdReg->cqeDmaAddr , dma ) ;
	setReg (cmdReg->cqIntVector , sync_dma.vector );
	setReg (cmdReg->isr_notify ,  sync_dma.irq_enabled );
	setReg (cmdReg->cmd , PCI_DMA_CQ_WRITE) ;
	
	reg cmdStatus ;
	while (1){
		getReg( cmdReg->cmdStatus , cmdStatus)
		if (cmdStatus == COMPLETE  ) break ;
		sys_yield_cpu();
	}
	
	sync_dma.status = 1;
	
	return 0;
}


uint32_t do_sync_rw_io_dma_operation(dma_t* dma, u32 op , void* req_obj ){

	u32 ix ;
	dma_op_t *e ;
	reg64 dma_addr;
	u8* next_ssd_addr;
	
	host_int_base_reg = get_dev_address(DEV_HOST_INT);
	cmdReg = (hostCmdReg *)host_int_base_reg ;
	
	next_ssd_addr = dma->buffer_addr ;
	
	VPRINTF_HOST_DMA(" rw io dma: req:%u, ssd_saddr:0x%x, tx_size:%u " ,
						  ((r_req_t*)req_obj)->index, next_ssd_addr , dma->data_size );
	
	for (ix = 0 ; ix < dma->n_ops ; ix++){
		e = &dma->list[ix];
		dma_addr = (reg64)e->host_p_addr ;
		
		VPRINTF_HOST_DMA_DETAILED ( "  dma %u/%u [ host_p_addr:{0x%x,0x%x}, ssd_addr:0x%x, len:%u ] ",
						ix+1, dma->n_ops ,
						dma_addr.Dwords.hbits, dma_addr.Dwords.lbits,
						next_ssd_addr, e->len);
		
		setReg64( cmdReg->dma_addr , dma_addr )
		setReg  ( cmdReg->size , e->len )
		setReg  ( cmdReg->buffMgtID , 0 )
		setReg  ( cmdReg->ssdAddr , next_ssd_addr )
		setReg  ( cmdReg->cmd , op )
		
		reg cmdStatus ;
		while (1){
			getReg( cmdReg->cmdStatus , cmdStatus)
			if (cmdStatus == COMPLETE  ) break ;
			sys_yield_cpu();
		}
		
		next_ssd_addr += e->len ;
	}
	
	dma->host_dma_complete = true;
	return 0;
}


uint32_t do_sync_dma_operation(dma_t* dma, u32 op ){//}, u8* ssd_addr, dma_list_t *list; ){
	
	uint32_t ix ;
	dma_op_t *e ;
	reg64 dma_addr;
	u8* next_ssd_addr;
	u32 r_len;				
	u32 tran_len ;
	 
	if (dma->c_op_len) r_len = dma->c_op_len ;
	else r_len = dma->data_size; 
	
	host_int_base_reg = get_dev_address(DEV_HOST_INT);
	cmdReg = (hostCmdReg *)host_int_base_reg ;
	
	next_ssd_addr = dma->buffer_addr ;
	
	VPRINTF_HOST_DMA(" Host dma [ transfer size:%u , dma->n_ops:%u ]"  , r_len , dma->n_ops  );
	
	
	for ( ix = dma->c_op_smem_pg ;
		 (ix < dma->n_ops) && r_len ; 
		 ix++, next_ssd_addr += tran_len , r_len -= tran_len)
	{
		e = &dma->list[ix]; 
		dma_addr = (reg64)e->host_p_addr ;
		tran_len = MIN(e->len , r_len );
		
		VPRINTF_HOST_DMA_DETAILED ( " dma %u/%u [ host_p_addr:{0x%x,0x%x}, ssd_addr:0x%x, len:%u ] ",
						ix+1, dma->n_ops ,
						dma_addr.Dwords.hbits, dma_addr.Dwords.lbits,
						next_ssd_addr, tran_len );
		
		setReg64( cmdReg->dma_addr , dma_addr )
		setReg  ( cmdReg->size , tran_len )
		setReg  ( cmdReg->buffMgtID , 0 )
		setReg  ( cmdReg->ssdAddr , next_ssd_addr )
		setReg  ( cmdReg->cmd , op )
		
		reg cmdStatus ;
		while (1){
			getReg( cmdReg->cmdStatus , cmdStatus)
			if (cmdStatus == COMPLETE  ) break ;
			sys_yield_cpu();
		}

	}
	
	dma->c_op_len = 0;
	dma->c_op_smem_pg = 0;
	dma->host_dma_complete = true;
	return 0;
}

void sync_dma_cbk(void * obj_ptr , uint32_t obj_type){
	
	switch (obj_type) {
		case PCI_DMA_SQ_READ:
			do_sync_read_scmd();
			break;
		case PCI_DMA_CQ_WRITE:
			do_sync_post_ccmd();
			break;
		case READ_IO_DMA_OPERATION :
			do_sync_rw_io_dma_operation(& ((r_req_t*)obj_ptr)->dma, PCI_DMA_WRITE, obj_ptr);
			break;
		case WRITE_IO_DMA_OPERATION:
			do_sync_rw_io_dma_operation(& ((w_req_t*)obj_ptr)->dma, PCI_DMA_READ, obj_ptr);
			break;
		case PCI_DMA_READ:
		case PCI_DMA_WRITE:
			do_sync_dma_operation(obj_ptr, obj_type);
			break;
		default:
			break;
	}
}






