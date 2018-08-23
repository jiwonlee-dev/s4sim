
#include "nvme_driver.h"
#include "host_dma.h"


#define AdminSQTailDB	(reg*)(host_int_base_reg + NVME_CTRL_OF + AdminDBTail_OFFSET )
#define AdminCQHeadDB	(reg*)(host_int_base_reg + NVME_CTRL_OF + AdminDBHead_OFFSET )
#define IdDataSize 4096
#define PrpListSize 8192


// variables for dumping statistics stats 
reg64 				ior		= {0x0000000000000000}; 
reg64 				iow		= {0x0000000000000000}; 
reg64 				iof		= {0x0000000000000000};
reg64 				ior_pgs	= {0x0000000000000000};
reg64 				iow_pgs	= {0x0000000000000000};

unsigned char		*host_int_base_reg;
hostCmdReg			*cmdReg ;
hostCtrlReg			*ctrlReg ;

NvmeCQueue 			cqs[MAX_IO_SQCQ_PAIRS + 1];
NvmeSQueue 			sqs[MAX_IO_SQCQ_PAIRS + 1];

uint32_t 			idleTimeCounter = 0 ;

u32 				hostMemPageSize ;
u32 				hostPageBits ;
uint32_t 			maxPrpEntries ;  
NvmeSCmd 			sCommand ;
NvmeCCmd 			cCommand;
uint32_t 			iosqCount = 0 ;
uint32_t 			iocqCount = 0 ;

// uncacheable buffers for admin request data  
NvmeIdController	*idCtrlBuf ;
NvmeIdNameSpace		*idNsBuf ;
uint32_t			*NvmeIdNSBuf ;

// prp list 
uint64_t			*prp_list;   

// buffer for scmd ccmd
NvmeSCmd 			*scmd_buf ;
NvmeCCmd 			*ccmd_buf ;

int 				check_door_ftn_id = 0;

 
static inline void inc_cq_tail(NvmeCQueue *cq)
{ 
	cq->tail++;
	if (cq->tail >= cq->size) {
		cq->tail = 0;
		cq->phase = !cq->phase;
	}
}

static inline void inc_sq_head(NvmeSQueue *sq)
{
	sq->head = (sq->head + 1) % sq->size;
}

static inline uint8_t nvme_cq_full(NvmeCQueue *cq)
{
	return (cq->tail + 1) % cq->size == cq->head;
}

int init_nvme_driver(){
	
	idCtrlBuf = (NvmeIdController*) get_uncacheable_ram(sizeof(NvmeIdController));
	idNsBuf = (NvmeIdNameSpace*) get_uncacheable_ram(sizeof(NvmeIdNameSpace));
	NvmeIdNSBuf = (uint32_t*)get_uncacheable_ram( IdDataSize / sizeof(uint32_t) ) ;
	prp_list = (uint64_t*)get_uncacheable_ram( PrpListSize );
	
	scmd_buf = (NvmeSCmd *) get_uncacheable_ram(sizeof(NvmeSCmd) * MAX_IO_SQCQ_ENTRIES);
	ccmd_buf = (NvmeCCmd *) get_uncacheable_ram(sizeof(NvmeCCmd) * MAX_IO_SQCQ_ENTRIES);
		
	// set the completed request callback function
	completed_req_cbk_ftn_id = sys_add_thread_function( NVME_DRIVER_THREAD , completed_req_cbk);
	
	// add a call back for initialization function
	int ftn_id = sys_add_thread_function( NVME_DRIVER_THREAD , initialize_nvme);
	sys_add_callback_ftn(ftn_id, 0 );
	
	// add check door bell function 
	check_door_ftn_id = sys_add_thread_function( NVME_DRIVER_THREAD , check_door_bell);

	return 0;
}

int shutdown_nvme_driver(){
	return 0;
}

void initialize_nvme(uint32_t arg_0){
	
	reg cmdStatus ;

	VPRINTF_STARTUP	("############      INIT NVMe      ############");
	
	host_int_base_reg = get_dev_address(DEV_HOST_INT);
	cmdReg = (hostCmdReg *)host_int_base_reg ;
	ctrlReg = (hostCtrlReg *)(host_int_base_reg + NVME_CTRL_OF);
	
	iocqCount = 0;
	iosqCount = 0;
	
	// establish connection with Host
	setReg (cmdReg->cmd ,  INIT );
	while(1){
		getReg( cmdReg->cmdStatus , cmdStatus)
		if ( cmdStatus == INITCOMPLETE){
			break ;
		}
		sys_yield_cpu();
	}
	VPRINTF_STARTUP(" check host connection") ;
	// check host connection status
	while(1){
		setReg ( cmdReg->cmd ,  HOST_CONNECTION_STATUS );
		getReg ( cmdReg->cmdStatus , cmdStatus)
		if ( cmdStatus == HOST_ATTACHED){
			break ;
		}
		//VPRINTF_STARTUP(" %x " , cmdStatus) ;
		sys_yield_cpu();
	}
	VPRINTF_STARTUP(" check host connection : OK ") ;

	setReg ( ctrlReg->cap_lo , 0x00000000  ) ;
	setReg ( ctrlReg->cap_hi , 0x00000000  ) ;
	setReg ( ctrlReg->vs , 0x00000000 )	;
	setReg ( ctrlReg->intmc , 0x00000000 ) ;	
	setReg ( ctrlReg->intms , 0x00000000 ) ;
	setReg ( ctrlReg->cc , 0x00000000 ) ;
	setReg ( ctrlReg->csts , 0x00000000 )
	
	// set ctrl register 
	union cap_lo_register	cap_lo;
	union cap_hi_register	cap_hi;
	
	cap_lo.raw = cap_hi.raw = 0 ;
	cap_lo.bits.mqes = 0x3 ; // 0xF ; // 0xFF ; // 0x3ff ;	 
	cap_lo.bits.cqr = 1 ;		 
	cap_lo.bits.ams = 1;			 
	cap_lo.bits.to = 0xf ;		 
	cap_hi.bits.css_nvm = 1 ;	
	cap_hi.bits.mpsmin = 0;		 
	cap_hi.bits.mpsmax = 4;	
	
	
	setReg ( ctrlReg->cap_lo , cap_lo.raw  ) ;
	setReg ( ctrlReg->cap_hi , cap_hi.raw ) ;
	setReg ( ctrlReg->vs , 0x00010100 )	;
	setReg ( ctrlReg->intmc , 0 ) ;	
	setReg ( ctrlReg->intms , 0 ) ;
	
	VPRINTF_STARTUP (" cap_hi.raw:%x,  cap_lo.raw :%x " , cap_hi.raw , cap_lo.raw);
	
	
	// wait for namespace to be configured before sending ready state to host
	VPRINTF_STARTUP (" waiting for namespace setup");
	while (1) {
		if(ssd_config.host_ds >= 9 ){ break ;}
		sys_yield_cpu();
	}
	VPRINTF_STARTUP (" namespace setup ftlv->NSCount:%u" , NUM_NS);
	

	
	// wait for host driver to set CC.EN bit to 1 
	VPRINTF_STARTUP(" ** Waiting for Host driver to set CC.EN=1 **");
	union cc_register cc ;
	uint32_t en = 0 ;
	while(1){
		getReg( ctrlReg->cc , cc.raw ) 
		en = cc.bits.en ;
		if (en){
			DPRINTF_MORE ("CC.EN=1 by Host driver ");
			break ;
		}
		sys_yield_cpu();
	}
	
	uint32_t cc_mps = cc.bits.mps ;
	hostPageBits = cc_mps + 12 ;
	hostMemPageSize =  0x1 << hostPageBits ;
	maxPrpEntries = hostMemPageSize / sizeof(uint64_t) ;
	VPRINTF_STARTUP ( " ** cc.mps=%u, hostPageBits:%u , hostMemPageSize=%u , maxPrpEntries:%u **" ,
					(unsigned int)cc_mps ,(unsigned int)hostPageBits, 
					(unsigned int)hostMemPageSize, (unsigned int)maxPrpEntries);
	
	union aqa_register aqa;
	reg64 asq , acq ;
	
	getReg (ctrlReg->aqa , aqa.raw ) ;
	getReg64(ctrlReg->asq , asq ) ;
	getReg64 (ctrlReg->acq , acq  ) ;
	VPRINTF_STARTUP ( " ** aqa.bits.asqs=%u , aqa.bits.acqs=%u  ,  asq addr={0x%x,0x%x} , acq addr={0x%x,0x%x} **" ,
					(unsigned int) aqa.bits.asqs , 
					(unsigned int) aqa.bits.acqs ,
					asq.Dwords.hbits , asq.Dwords.lbits , 
					acq.Dwords.hbits , acq.Dwords.lbits);
	
	// init admin completion queue
	NvmeCQueue *cq = &cqs[0];
	cq->hostAddr = acq.raw;
	cq->cqid = 0 ;
	cq->phase = 1; 
	cq->size = (uint32_t) aqa.bits.acqs + 1;
	cq->irq_enabled = 1 ;
	cq->vector = 0;
	cq->head = cq->tail = 0;
	cq->headBDReg = AdminCQHeadDB ;
	cq->pending.head = 0;
	cq->pending.tail = 0;
	cq->pending.size = cq->size;
	
	// init admin submission queue
	NvmeSQueue *sq = &sqs[0];
	sq->hostAddr = asq.raw;
	sq->sqid = 0 ;
	sq->size = (uint32_t) aqa.bits.asqs  + 1;
	sq->cqid = 0 ;
	sq->head = sq->tail = 0;
	
	// register admin cq interrupt vector
	setReg( cmdReg->cqIntVector , 0 );
	setReg( cmdReg->cmd , PCI_ENABLE_MSIX_VECTOR );
	
	// set CSTS.RDY bit 
	DPRINTF_MORE ("write csts \n") ;	
	union csts_register csts;
	getReg( ctrlReg->csts , csts.raw ) ;  
	csts.bits.rdy = 1;
	setReg ( ctrlReg->csts , csts.raw )
	
	/** end of controller initialization */
	
	// add a call back to check door bells
	sys_add_callback_ftn(check_door_ftn_id, 0 );
}






#ifdef VERBOSE_NVME_IO  
static inline void vprint_request (NvmeSQueue *sq, u32 sqe , NvmeIOSCmd *scmd, uint32_t req_index){
	
	VPRINTF_NVME_IO("%s IO: req:%u, sqid:%u, sqe:%u, cid:%u, nsid:%u, slba:%u, nlbs:%u, data_size:%u, prp1 {0x%x,0x%x}, prp2 {0x%x,0x%x} ",
					
					io_opcode_string[scmd->opcode] ,
					req_index,
					(unsigned int)sq->sqid , 
					(unsigned int)sqe,
					(unsigned int)scmd->cid ,
					(unsigned int)scmd->nsid ,
					(unsigned int)scmd->slba ,
					(unsigned int)scmd->nlbs + 1 , 
					(unsigned int)(scmd->nlbs + 1) << HOST_DS ,
					(unsigned int)((reg64)scmd->prp1).Dwords.hbits,
					(unsigned int)((reg64)scmd->prp1).Dwords.lbits,
					(unsigned int)((reg64)scmd->prp2).Dwords.hbits,
					(unsigned int)((reg64)scmd->prp2).Dwords.lbits 
					)
}

static inline void vprint_ccmd_post(NvmeCQueue *cq, NvmeCCmd *ccmd, uint32_t opcode, uint32_t req_index){
	
	char* str ;
	if(cq->cqid == 0){ str = "Admin" ;}
	else {str = io_opcode_string[opcode] ;}
	
	VPRINTF_NVME_IO ("Post %s: req:%u, cqid:%u, tail:%u, head:%u, cid:%u, status:%u, irq:%u, intVector:%u, ssd_addr:0x%x",
					  str,
					 (unsigned int)req_index,(unsigned int)cq->cqid ,
					 (unsigned int )cq->tail , (unsigned int)cq->head,
					 (unsigned int )ccmd->cid, (unsigned int)(ccmd->status_p >> 1),						 
					 (unsigned int )cq->irq_enabled , (unsigned int)cq->vector,
					 (unsigned int )ccmd );

}

static inline void vprint_isp_ccmd_post(NvmeCQueue *cq, NvmeCCmd *ccmd, uint32_t req_index){
	
#if defined(VERBOSE_ISP_IO_DETAILED) || defined(VERBOSE_NAND_IO)
	VPRINTF_NVME_IO ("Post ISP IO: req:%u, cqid:%u, tail:%u, head:%u, cid:%u, status:%u, irq:%u, intVector:%u, ssd_addr:0x%x",
					 (unsigned int)req_index,(unsigned int)cq->cqid ,
					 (unsigned int )cq->tail , (unsigned int)cq->head,
					 (unsigned int )ccmd->cid, (unsigned int)(ccmd->status_p >> 1),						 
					 (unsigned int )cq->irq_enabled , (unsigned int)cq->vector,
					 (unsigned int )ccmd );
#endif

}

 
#else 
static inline void vprint_request (NvmeSQueue *sq, u32 sqe , NvmeIOSCmd *scmd, uint32_t req_index){}

static inline void vprint_ccmd_post(NvmeCQueue *cq, NvmeCCmd *ccmd, uint32_t opcode, uint32_t req_index){}

static inline void vprint_isp_ccmd_post(NvmeCQueue *cq, NvmeCCmd *ccmd, uint32_t req_index){}
#endif










static inline void enqueue_admin_ccmd(a_req_t *req ){
	
	NvmeCQueue *cq = &cqs[0];
	// push to fifo queue	
	cq->pending.objs[cq->pending.tail] = (void*)req ;
	cq->pending.tail++; 
	cq->pending.tail = cq->pending.tail % cq->pending.size ; 
	
	post_admin_ccmds();
}

void post_admin_ccmds(){
	
	NvmeCQueue *cq = &cqs[0];
	NvmeSQueue *sq = &sqs[0];
	
	while (1){
		
		// check for pending completed request 
		if (cq->pending.head == cq->pending.tail) { return ; } 
		
		// check if host cq if full
		if(nvme_cq_full(cq)){
			VPRINTF_NVME_ADMIN ("*** Host Completion Queue Full: cqid:%u, tail:%u, head:%u ***",
								  (unsigned int) cq->cqid ,
								  (unsigned int) cq->tail , 
								  (unsigned int) cq->head );
			return;
		}
		
		AdminRequest *req = (AdminRequest*) cq->pending.objs[cq->pending.head]; 
		cq->pending.head++ ; 		
		cq->pending.head = cq->pending.head % cq->pending.size ; 

		// setup completion command data
		NvmeCCmd *ccmd = ccmd_buf;
		ccmd->cdw0 = req->ccmd_cdw0.raw;
		ccmd->sqhd = sq->head ;
		ccmd->cid  = req->scmd.cid ;
		ccmd->status_p = (req->status << 1) | cq->phase  ;
		
		vprint_ccmd_post(cq, ccmd, req->scmd.opcode, req->index );		
			 
		//VPRINTF_NVME_ADMIN(" -- post ccmd  -- ");
		sync_post_ccmd(cq->cqid, cq->tail, cq->hostAddr, (u8*)ccmd,
					   1,cq->vector ,cq->irq_enabled );
		req->req_stage = CCMD_POSTED ;

		// increase the tail value 
		inc_cq_tail(cq);
		sys_free_req_obj(req);
	}
}

void enqueue_io_ccmd(uint32_t cqid , io_req_common_t *req ){

	NvmeCQueue *cq = &cqs[cqid];

	// check if pending queue is full
	if (cq->pending.head == ((cq->pending.tail + 1) % cq->pending.size) ) { 
		VPRINTF_ERROR (" *** Pending Ccmd Queue Full : cqid:%u ***" , cqid );
		// TODO , come up with another pending queue for this, probably a linked list
		return ;		 
	}

	// push to fifo queue	
	cq->pending.objs[cq->pending.tail] = req ;
	cq->pending.tail++; 
	cq->pending.tail = cq->pending.tail % cq->pending.size ; 
	
	req->req_stage = PENDING_CCMD_POST;

	post_io_ccmds(cqid);
}

void post_io_ccmds(uint32_t cqid){
	
	NvmeCQueue 	*cq = &cqs[cqid];
	NvmeSQueue 	*sq ;
	NvmeCCmd 	*ccmd = ccmd_buf;
		
	
	while (1){
		
		// check for pending completed request 
		if (cq->pending.head == cq->pending.tail) { return ; } 
		
		// check if host cq if full
		if(nvme_cq_full(cq)){
			VPRINTF_NVME_IO_DETAILED ("*** Host Completion Queue Full: cqid:%u, tail:%u, head:%u ***",
								(unsigned int) cq->cqid ,
								(unsigned int) cq->tail , 
								(unsigned int) cq->head );
			return;
		}
		
		io_req_common_t* io_req = cq->pending.objs[cq->pending.head]; 
		cq->pending.head++ ; 		
		cq->pending.head = cq->pending.head % cq->pending.size ; 
		
		// setup completion command data
		sq = &sqs[io_req->sqid];
		ccmd->cid  = io_req->scmd.cid ;
		ccmd->sqhd = sq->head ;
		ccmd->status_p = (io_req->status << 1) | cq->phase  ;
		ccmd->cdw0 = io_req->ccmd_dw0.raw ;
		
		vprint_ccmd_post(cq, ccmd, io_req->scmd.opcode, io_req->index );


/*
		if (io_req->type == ISP_IO_REQUEST){
			VPRINTF ("Post ISP IO: req:%u, cqid:%u, tail:%u, head:%u, cid:%u, status:%u, irq:%u, intVector:%u, ssd_addr:0x%x",
					 (unsigned int)io_req->index,(unsigned int)cq->cqid ,
					 (unsigned int )cq->tail , (unsigned int)cq->head,
					 (unsigned int )ccmd->cid, (unsigned int)(ccmd->status_p >> 1),						 
					 (unsigned int )cq->irq_enabled , (unsigned int)cq->vector,
					 (unsigned int )ccmd );
		}
		*/

		// post command		
		sync_post_ccmd(cq->cqid, cq->tail, cq->hostAddr, (u8*)ccmd,
						   1,cq->vector ,cq->irq_enabled );
		
		io_req->req_stage = CCMD_POSTED ;

		// increase the tail value 
		inc_cq_tail(cq);

		// reset request object
		sys_free_req_obj(io_req);
	}
}


dma_t dma ;
static inline void get_prp_page(u64 host_p_addr , u8* ssd_addr, u32 len ){
	dma.n_ops = 1;
	dma.data_size = len ;
	dma.buffer_addr = ssd_addr ;
	dma.list[0].host_p_addr = host_p_addr;
	dma.list[0].len = len ;
	
	sync_host_dma(&dma, PCI_DMA_READ,0,0); 
}

static inline void add_dma_entry(dma_t* dma_obj, u64 host_p_addr, u32 len ){
	dma_obj->list[dma_obj->n_ops].host_p_addr = host_p_addr ;
	dma_obj->list[dma_obj->n_ops].len = len ;
	dma_obj->n_ops++;
}


int decode_prp(u64 prp1, u64 prp2, dma_t *dma_obj ) {
	
	if (!prp1) { return  -1;}
	
	u32 len = dma_obj->data_size ;
	dma_obj->n_ops = 0 ;

	uint32_t trans_len = hostMemPageSize - (prp1 % hostMemPageSize);
	trans_len = MIN(len, trans_len);
		
	add_dma_entry(dma_obj , prp1 , trans_len ) ; 
	len -= trans_len;
	if (len) {
		if (!prp2) {
			goto unmap;
		}
		if (len > hostMemPageSize) {
			uint32_t nents, prp_trans;
			int i = 0;
			
			nents = (len + hostMemPageSize - 1) >> hostPageBits;
			prp_trans = MIN( maxPrpEntries, nents) * sizeof(uint64_t);
			get_prp_page(prp2, (u8*)prp_list,prp_trans) ; 
			
			while (len != 0) {
				uint64_t prp_ent = prp_list[i] ;
				
				if (i == maxPrpEntries - 1 && len > hostMemPageSize ) {
					if (!prp_ent || prp_ent & ( hostMemPageSize - 1)) {
						goto unmap;
					}
					
					i = 0;
					nents = (len + hostMemPageSize - 1) >> hostPageBits ;
					prp_trans = MIN(maxPrpEntries , nents) * sizeof(uint64_t);
					get_prp_page (prp_ent, (u8*)prp_list , prp_trans ) ; 
					
					prp_ent = prp_list[i] ;
				}
				
				if (!prp_ent || prp_ent & (hostMemPageSize - 1)) {
					goto unmap;
				}
				
				trans_len = MIN(len, hostMemPageSize);
				add_dma_entry(dma_obj , prp_ent , trans_len);
				len -= trans_len;
				i++;
			}
		} 
		else {
			if (prp2 & ( hostMemPageSize - 1)) {
				goto unmap;
			}
			add_dma_entry(dma_obj , prp2 , len ) ;
		}
	}
	
#ifdef VERBOSE_HOST_DMA_DETAILED
	VPRINTF ( "\tprp list , datasize:%u, n_ops:%u", dma_obj->data_size, dma_obj->n_ops ); 
	for (u32 ix = 0 ; ix < dma_obj->n_ops ; ix++){
		reg64 dma_addr = (reg64)dma_obj->list[ix].host_p_addr ;
		VPRINTF ("\t\t %u host_p_addr:{0x%x,0x%x} len:%u", 
				ix+1, dma_addr.Dwords.hbits, dma_addr.Dwords.lbits, dma_obj->list[ix].len);
	}
#endif
	
	return 0;
unmap:
	return -1 ; 
}

void do_sync_dma(u64 prp1, u64 prp2, dma_t* dma, u8* ssd_addr, u32 dma_operation){
	decode_prp(prp1, prp2, dma);
	dma->buffer_addr = ssd_addr ;
	dma->c_op_smem_pg = 0;
	dma->c_op_len = 0;
	sync_host_dma(dma, dma_operation,0,0 );
}

uint16_t nvme_identify(AdminRequest *req ){
	
	NvmeIdentifyCmd * cmd = (NvmeIdentifyCmd*)&(req->scmd) ;
	uint32_t mdts_cal;

	switch  (cmd->cns) {
		
		case 0x01 :
		{
			NvmeIdController *id = idCtrlBuf ;
			
			//TODO : get this value form Qemu , for several execution of unmodified qemu , id->vid is always = 0x8086
			id->vid = 0x8086 ;	
			
			//TODO : get this value form Qemu , for several execution of unmodified qemu , id->ssvid is always = 0x1af4
			id->ssvid = 0x1af4 ;	
			
			strpadcpy((char *)id->mn, sizeof(id->mn), "ESOS/SnP NVMe Ctrl", ' ');
			strpadcpy((char *)id->fr, sizeof(id->fr), "1.0", ' ');
			strpadcpy((char *)id->sn, sizeof(id->sn), "Gem5iSSD", ' ');
			id->rab = 6;
			id->ieee[0] = 0x00;
			id->ieee[1] = 0x02;
			id->ieee[2] = 0xb3;
			id->oacs = 0 ; 
			id->frmw = 7 << 1;
			id->lpa = 1 << 0;
			id->sqes = (0x6 << 4) | 0x6;
			id->cqes = (0x4 << 4) | 0x4;
			id->nn = NUM_NS
			;  
			id->oncs = 0x0000 ;// 0x0004 ;			// set bit 3 to "1" to indicate trim is supported 
			id->psd[0].mp = 0x9c4 ;  
			id->psd[0].enlat = 0x10 ; 
			id->psd[0].exlat = 0x4 ;  
			id->vwc = 1;
			
			// set maximum data transfer
			mdts_cal =  MDTS_SIZE >> 12;
			id->mdts = 0;
			while (mdts_cal > 1 ) {
				mdts_cal = mdts_cal >> 1 ;
				id->mdts++;
			}
			
			VPRINTF_NVME_ADMIN("Admin Cmd : Identify controller [id->nn:%u, id->mdts:%u (max data transfer size:%u ) ]" , 
							(unsigned int )id->nn, (unsigned int )id->mdts , (unsigned int )MDTS_SIZE)
			
			req->dma.data_size = sizeof(NvmeIdController);
			do_sync_dma(req->scmd.prp1, req->scmd.prp2, &req->dma , (u8*)id, PCI_DMA_WRITE);
			
			// post completion command 
			enqueue_admin_ccmd(req);
		}
		break;
			
		case 0x00:
		{
			// set name space parameters  
			NvmeIdNameSpace *id_ns = idNsBuf ;
			
			id_ns->nsfeat = 0;
			id_ns->nlbaf = 0;
			id_ns->flbas = 0;
			id_ns->mc = 0;
			id_ns->dpc = 0;
			id_ns->dps = 0;
			id_ns->lbaf[0].ds = HOST_DS  ; 
			id_ns->ncap = (uint64_t)ssd_config.ncap ;
			id_ns->nuse = (uint64_t)ssd_config.nuse ;
			id_ns->nsze = (uint64_t)ssd_config.nsze;
			
			VPRINTF_NVME_ADMIN("Admin Cmd : Identify namespace [nsid:%u , ds:%u , ncap:%u , nuse:%u , nzse:%u ] "  ,
						   (unsigned int)cmd->nsid,
						   (unsigned int)id_ns->lbaf[0].ds ,
						   (unsigned int)id_ns->ncap , 
						   (unsigned int)id_ns->nuse ,
						   (unsigned int)id_ns->nsze );
			
			req->dma.data_size = sizeof(NvmeIdNameSpace);
			do_sync_dma(req->scmd.prp1, req->scmd.prp2, &req->dma , (u8*)id_ns, PCI_DMA_WRITE);
			
			// post completion command 
			enqueue_admin_ccmd(req);
		}
		break ;
			
		case 0x02:
		{
			VPRINTF_NVME_ADMIN("Admin Cmd : Identify active namespace IDs nsid:%u " , (unsigned int) cmd->nsid );
			
			uint32_t min_nsid = cmd->nsid ;
			int i, j = 0;
			memset32( NvmeIdNSBuf , 0 , IdDataSize / sizeof(uint32_t) );
			
			for (i = 0; i <  NUM_NS ; i++) {
				if (i < min_nsid) {
					continue;
				}
				NvmeIdNSBuf[j++] = (i + 1);
				if (j == IdDataSize / sizeof(uint32_t)) {
					break;
				}
			}
			
			req->dma.data_size = IdDataSize ;
			do_sync_dma(req->scmd.prp1, req->scmd.prp2, &req->dma , (u8*)NvmeIdNSBuf, PCI_DMA_WRITE);
			
			// post completion command 
			enqueue_admin_ccmd(req);
		}
			break;
	}
	
	return 1;
}

uint16_t nvme_set_feature(AdminRequest *req ){
	
	NvmeSetFeatureCmd * cmd = (NvmeSetFeatureCmd*) &(req->scmd) ;
	 
	switch (cmd->fid) 
	{
		case 0x7:
			
			VPRINTF_NVME_ADMIN("Admin Cmd : Set Feature : Number of Queues : NSQR:%u  , NCQR:%u  \n" , 
						  (unsigned int )cmd->nsq + 1 ,
						  (unsigned int )cmd->ncq + 1 );
			
			req->ccmd_cdw0.words.w0 = MIN(cmd->nsq , MAX_IO_SQCQ_PAIRS)  ;
			req->ccmd_cdw0.words.w1 = MIN(cmd->ncq , MAX_IO_SQCQ_PAIRS)  ; 
			
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
			
		case 0x6 :
			
			VPRINTF_NVME_ADMIN ( "Admin Cmd : Set Feature : WCE  \n"    );
			
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
			
		default:
			VPRINTF_ERROR ("\n ********  unimplemented Set Feature FID:0x%x ***********\n" , 
						   (unsigned int ) cmd->fid );
			
			req->status = NVME_SC_INVALID_FIELD | NVME_SC_DNR ;
			
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
	}
	
	return 0;
}

uint16_t nvme_get_feature(AdminRequest * req){
	
	NvmeGetFeatureCmd * cmd =(NvmeGetFeatureCmd*) &(req->scmd) ;
			
	switch (cmd->fid) 
	{
		case 0x7:
			VPRINTF_NVME_ADMIN("Admin Cmd : Get Feature : Number of Queues : CQ:%u, SQ:%u  \n" , 
						   (unsigned int)iocqCount , (unsigned int)iosqCount  );
			
			req->ccmd_cdw0.words.w0 =  iosqCount;
			req->ccmd_cdw0.words.w1 =  iocqCount  ; 
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
			
		case 0x6 :

			VPRINTF_NVME_ADMIN("Admin Cmd : Get Feature : WCE  \n");
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
			
		default:
			
			VPRINTF_ERROR ("\n ********  unimplemented Get Feature FID:0x%x ***********\n" , 
						   (unsigned int) cmd->fid );
			
			req->status = NVME_SC_INVALID_FIELD | NVME_SC_DNR ;
			// post completion command 
			enqueue_admin_ccmd(req);
			break;
	}
	
	return 0 ;	
}



uint16_t nvme_fw_download(AdminRequest * req){
	
	NvmefwdlCmd * cmd = (NvmefwdlCmd*) &(req->scmd) ;
	req->dma.data_size = ((unsigned int)cmd->num_dwords + 1) << 2 ;
	u8* ssd_addr = external_code_buffer + ((unsigned int)cmd->dword_off << 2) ;
	
	do_sync_dma(cmd->prp1, cmd->prp2, &req->dma, ssd_addr, PCI_DMA_READ);
	
	external_code_size = ((unsigned int)cmd->dword_off << 2) + req->dma.data_size ;	
	VPRINTF_NVME_ADMIN ( "Admin Cmd :  firmware or ISP binary download :  No of Dwords:%u, Dword offset:%u , code size:%u ",
						(unsigned int)cmd->num_dwords ,
						(unsigned int)cmd->dword_off,
						(unsigned int)external_code_size );
	
	// post completion command 
	enqueue_admin_ccmd(req);
	return 0 ;
}

uint16_t nvme_fw_activate(AdminRequest * req){
	
	VPRINTF_NVME_ADMIN ( "Admin Cmd : firmware or ISP binary activate : final code size:%u, firmware slot:%u, activate action:%u ",
						(unsigned int)external_code_size,
						(unsigned int)((NvmefwActCmd*)&(req->scmd))->fs ,
						(unsigned int)((NvmefwActCmd*)&(req->scmd))->aa );
	
	add_applet((void *) external_code_buffer , external_code_size );
	
	// post completion command 
	enqueue_admin_ccmd(req);
	return 0;
}


uint16_t nvme_create_sq(AdminRequest *req)
{ 
	NvmeCreateSQCmd * cmd = (NvmeCreateSQCmd*) &(req->scmd) ;
	uint32_t index = cmd->sqid ;
	uint32_t tailDBoffset = AdminDBTail_OFFSET + (index * 0x8 ) ;
	
	VPRINTF_STARTUP ( "Admin Cmd : Create IO Submission Queue : ID:%u, Size:%u, pc:%u, qprio:%u, cqid:%u, BaseAddr:%#lx, DBoffset:0x%x",
					(unsigned int) index , 
					(unsigned int)cmd->size ,
					(unsigned int)cmd->pc , 
					(unsigned int)cmd->qprio,
					(unsigned int)cmd->cqid , 
					(long unsigned int)cmd->prp1 , 
					(unsigned int) tailDBoffset);
	
	sqs[index].sqid = index ;
	sqs[index].size = cmd->size +1;
	sqs[index].cqid = cmd->cqid ;
	sqs[index].hostAddr = cmd->prp1 ;
	sqs[index].head = 0;
	sqs[index].tail = 0;
	sqs[index].tailBDReg = (reg*)(host_int_base_reg + NVME_CTRL_OF + tailDBoffset ) ;

	iosqCount++;	
	
	// post completion command 
	enqueue_admin_ccmd(req);
	return index  ;  
}

uint16_t nvme_create_cq(AdminRequest *req ){
	
	NvmeCreateCQCmd * cmd = (NvmeCreateCQCmd*) &(req->scmd);
	uint32_t index = cmd->cqid ;
	uint32_t db = AdminDBHead_OFFSET + (index * 0x8 ) ;
	
	VPRINTF_STARTUP("Admin Cmd : Create IO Completion Queue : ID:%u, Size:%u, pc:%u, ien:%u, ivector:%u, headDBOffset:0x%x" ,
				   (unsigned int)cmd->cqid ,
				   (unsigned int)cmd->size ,
				   (unsigned int)cmd->pc , 
				   (unsigned int)cmd->ien , 
				   (unsigned int)cmd->ivector ,
				   (unsigned int) db);
	
	cqs[index].cqid = cmd->cqid ;
	cqs[index].phase = 1; 
	cqs[index].size = cmd->size + 1;
	cqs[index].hostAddr = cmd->prp1 ;
	cqs[index].irq_enabled = cmd->ien;
	cqs[index].vector = cmd->ivector;
	cqs[index].head = 0;
	cqs[index].tail = 0;
	cqs[index].headBDReg = (reg*)(host_int_base_reg + NVME_CTRL_OF + db ) ;
	cqs[index].pending.head = 0;
	cqs[index].pending.tail = 0;
	cqs[index].pending.size = cqs[index].size;
	// register cq interrupt vector
	if (cqs[index].irq_enabled){
		setReg( cmdReg->cqIntVector , cqs[index].vector );
		setReg( cmdReg->cmd , PCI_ENABLE_MSIX_VECTOR );
	}
	
	iocqCount ++;
	
	// post completion command 
	enqueue_admin_ccmd(req);
	return index ;
}


uint16_t nvme_delete_sq (AdminRequest *req){
	
	NvmeCreateSQCmd *cmd = (NvmeCreateSQCmd*) &(req->scmd);
	uint32_t index = cmd->sqid ;
	
	VPRINTF_NVME_ADMIN( "\n Command : Delete IO Submission Queue : ID:%u" , index );
	
	sqs[index].sqid = 0 ;
	sqs[index].size = 0;
	sqs[index].hostAddr = 0 ;
	sqs[index].head =  0;
	sqs[index].tailBDReg = 0;
	
	iosqCount--;
	// post completion command 
	enqueue_admin_ccmd(req);
	return 0;
}

uint16_t nvme_delete_cq (AdminRequest *req ) {
	
	NvmeCreateCQCmd * cmd = (NvmeCreateCQCmd*)&(req->scmd);
	uint32_t index = cmd->cqid ;
	
	VPRINTF_NVME_ADMIN ( "Command : Delete IO Completion Queue : ID:%u" , index );
	
	cqs[index].cqid = 0;
	cqs[index].phase = 0; 
	cqs[index].size =0 ;
	cqs[index].hostAddr = 0;
	cqs[index].irq_enabled = 0;
	cqs[index].vector = 0;
	cqs[index].head = 0;
	cqs[index].headBDReg = 0 ;
	
	iocqCount --;
	// post completion command 
	enqueue_admin_ccmd(req);
	return 0;
}

uint16_t nvme_adort_cmd(AdminRequest * req){
	
	dword_t cdw10;
	cdw10.raw = req->scmd.cdw10;
	
	io_req_common_t *io_req = sys_find_io_request((unsigned int )cdw10.words.word_0, (unsigned int )cdw10.words.word_1) ;

	VPRINTF_ERROR ("****  Abort command: SQID:%u , CID:%u REQ:{ index:%u, type:%s, stage:%s } ****\n" , 
				   (unsigned int )cdw10.words.word_0, 
				   (unsigned int )cdw10.words.word_1,
				   io_req->index ,
				   request_type_string[io_req->type],
				   request_stage_string[io_req->req_stage]
				   );
	
	sys_get_active_req_objects();

	// post completion command 
	enqueue_admin_ccmd(req);

	// TODO : stop any ongoing operation 
	return 0;
}

uint16_t nvme_unimplemented_cmd(AdminRequest * req){
	
	VPRINTF_ERROR ("********  unimplemented Admin opcode:0x%x ********" , (unsigned int) req->scmd.opcode);
	req->status = NVME_SC_INVALID_OPCODE | NVME_SC_DNR ;
	// post completion command 
	enqueue_admin_ccmd(req);
	return 0;
}

int process_admin_cmd(){
	// copy scmd 
	uint64_t host_p_addr = sqs[0].hostAddr ;//+ (sqe * S_CMD_FORMAT_SIZE ) ;
	NvmeSCmd *scmd = &(scmd_buf[sqs[0].head]);
	AdminRequest *req = (a_req_t*)sys_malloc_req_obj(ADMIN_REQUEST);
	
	//VPRINTF_NVME_ADMIN(" -- get scmd  -- ");
	sync_read_scmd(0, sqs[0].head, host_p_addr, (u8*)scmd, 1);
	//VPRINTF_NVME_ADMIN(" -- get scmd done op:%x-- ", scmd->opcode );
	req->status = 0;
	req->ccmd_cdw0.raw = 0;
	// copy relevant submission command fields to request data structure 
	req->scmd_entry = sqs[0].head  ;
	nvmescmd_cp * to = (nvmescmd_cp*)&req->scmd;
	nvmescmd_cp * from = (nvmescmd_cp*)scmd ;
	to->cdw0_1 = from->cdw0_1;
	to->prp1 	= from->prp1;
	to->prp2 	= from->prp2;
	to->cdw10_11 = from->cdw10_11;
	
	inc_sq_head(&sqs[0]); // increament the queue head 
	
	switch (req->scmd.opcode) {
			
		case NVME_OPC_IDENTIFY:
			nvme_identify(req); 
			break;
		case NVME_OPC_SET_FEATURES:
			nvme_set_feature(req); 
			break ;
		case NVME_OPC_GET_FEATURES:
			nvme_get_feature( req);
			break;
		case NVME_OPC_CREATE_IO_SQ:
			nvme_create_sq(req); 
			break;
		case NVME_OPC_CREATE_IO_CQ:
			nvme_create_cq(req); 
			break ;
		case NVME_OPC_DELETE_IO_SQ:
			nvme_delete_sq(req);
			break;
		case NVME_OPC_DELETE_IO_CQ:
			nvme_delete_cq(req);
			break;
		case NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD:
			nvme_fw_download(req);
			break ;
		case NVME_OPC_FIRMWARE_ACTIVATE :
			nvme_fw_activate(req);
			break;
		case NVME_OPC_ABORT:
			nvme_adort_cmd(req);
			break ;			
		default:
			nvme_unimplemented_cmd(req);
			break ;
	}
	
	return 0 ;
}


int verify_rw_param(NvmeSQueue *sq, NvmeIOSCmd* scmd){
	
	if ( (scmd->nsid ) > NUM_NS){
		VPRINTF_ERROR ( " ERROR Namaspace:%u does not exit" , scmd->nsid ) ;
		//req->status = ; //TODO
		return -1 ;
	}
	
	if ((scmd->slba + scmd->nlbs) > ssd_config.nsze) {
		VPRINTF_ERROR ( " Requested page is beyond namespace page range [nsid:%u slba:%u, nlbs:%u ]" , 
					   (unsigned int )scmd->nsid ,
					   (unsigned int )scmd->slba,
					   (unsigned int )scmd->nlbs) ;
		//req->status = ; //TODO
		return  -1;
	}
	
	if ( MDTS_PAGES <= scmd->nlbs ) {
		VPRINTF_ERROR ( " IO data size > MDTS  [nsid:%u slba:%u, nlbs:%u datasize:%u ]" , 
					   (unsigned int )scmd->nsid,
					   (unsigned int )scmd->slba, 
					   (unsigned int )scmd->nlbs + 1 ,
					   (unsigned int )((scmd->nlbs+1) << HOST_DS) ) ;
		//req->status = ; //TODO
		return  -1;
	}
	
	// everything seems fine
	return 0;
}

// push read request to pending buffer queue
#define R_BUFFER_Q_SIZE MAX_IO_SQCQ_ENTRIES
uint32_t r_buf_q_tail=0;
uint32_t r_buf_q_head=0;
bool r_buf_q_empty=true;
r_req_t* r_buf_q_objs[R_BUFFER_Q_SIZE];

//static inline 
void push_r_buf_q(r_req_t* req){
	r_buf_q_objs[r_buf_q_tail] = req;
	r_buf_q_tail++;
	r_buf_q_tail = r_buf_q_tail % R_BUFFER_Q_SIZE ;
	r_buf_q_empty=false;
}

//static inline 
r_req_t* pop_r_buf_q(){
	uint32_t head = r_buf_q_head;
	r_buf_q_head ++;
	r_buf_q_head = r_buf_q_head % R_BUFFER_Q_SIZE ;
	if (r_buf_q_head == r_buf_q_tail){ r_buf_q_empty=true ;}
	return r_buf_q_objs[head];
}

#define W_BUFFER_Q_SIZE MAX_IO_SQCQ_ENTRIES
uint32_t w_buf_q_tail=0;
uint32_t w_buf_q_head=0;
bool w_buf_q_empty=true;
w_req_t* w_buf_q_objs[W_BUFFER_Q_SIZE];

void push_w_buf_q(w_req_t* req){
	w_buf_q_objs[w_buf_q_tail] = req;
	w_buf_q_tail++;
	w_buf_q_tail = w_buf_q_tail % W_BUFFER_Q_SIZE ;
	w_buf_q_empty=false;
}

w_req_t* pop_w_buf_q(){
	uint32_t head = w_buf_q_head;
	w_buf_q_head ++;
	w_buf_q_head = w_buf_q_head % W_BUFFER_Q_SIZE ;
	if (w_buf_q_head == w_buf_q_tail){ w_buf_q_empty=true ;}
	return w_buf_q_objs[head];
}

void process_read_ios_step_2(){
	
	//VPRINTF_STARTUP (" %s " , __FUNCTION__);

	while (!r_buf_q_empty){
		
		// pick the head obj without poping it
		r_req_t *req = r_buf_q_objs[r_buf_q_head];
		
		//VPRINTF_STARTUP (" [ %s ] head req:%u " , __FUNCTION__ , req->index );
		
		// assign page buffers 
		if(get_read_io_buffer(req) == -1){
			return ;
		}
		
		pop_r_buf_q();
		
		// push to both FTL
		sys_push_to_ftl((void*)req,READ_IO_REQUEST);
	} 
	
	//VPRINTF_STARTUP (" %s noo more pending r req " , __FUNCTION__);
	
}

static inline io_req_common_t* setup_io_req(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd, u32 req_type){
	
	NvmeSQueue 	*sq ;
	NvmeCQueue 	*cq ;
	io_req_common_t *req ;
	
	sq = &sqs[sqid] ;
	cq = &cqs[sq->cqid];
	req = (io_req_common_t*)sys_malloc_req_obj(req_type);
	
	// copy common nvme io fields and other values to request data structure
	req->status = 0;
	req->sqid = sqid;
	req->cqid = cq->cqid;
	req->sqe = sqe ;
	req->scmd.cid  = scmd->cid ;
	req->scmd.nsid = scmd->nsid ;
	req->scmd.prp1 = scmd->prp1;
	req->scmd.prp2 = scmd->prp2;
	
	return req;
}

void process_read_io(uint32_t sqid, u32 sqe , NvmeIOSCmd * scmd){
	
	NvmeSQueue *sq = &sqs[sqid] ;
	NvmeCQueue *cq = &cqs[sq->cqid];
	r_req_t *req = (r_req_t*)sys_malloc_req_obj(READ_IO_REQUEST);
	
	// copy relevant fields to request data structure 
	//req->data_size = (scmd->nlbs + 1) << HOST_DS ;
	req->status = 0;
	req->sqid = sqid;
	req->cqid = cq->cqid;
	req->sqe = sqe ;
	nvmescmd_cp * to = (nvmescmd_cp*)&req->scmd;
	nvmescmd_cp * from = (nvmescmd_cp*)scmd ;
	to->cdw0_1 	= from->cdw0_1;
	to->prp1 	= from->prp1;
	to->prp2 	= from->prp2;
	to->cdw10_11 = from->cdw10_11;
	to->cdw12_13 = from->cdw12_13;
	to->cdw14_15 = from->cdw14_15;
	
	if(verify_rw_param(sq, scmd) == -1){
		// post completion command
		enqueue_io_ccmd (req->cqid,(io_req_common_t*)req);
		return;
	}
	
	LOG_EVENT(0 , LOG_READ_IO ) ;

	//volatile u32 *S = (u32*)0x2D419000 ;
	//*S = LOG_READ_IO ; 

	//lpn_t lpn = req->scmd.slba ; 
	req->nand_io_complete = false;
	req->dma.host_dma_complete = false;
	req->dma.data_size = (req->scmd.nlbs+1) << HOST_DS ;
	
	//for ( uint32_t i =0; i <= req->scmd.nlbs ; i++ ){
		//req->pages[i].lpn = lpn ++ ;
	//	req->pages[i].nand_io_complete = false ;
	//	req->pages[i].host_dma_complete = false ;
	//}
	
	// decode the prp list 
	decode_prp(req->scmd.prp1 , req->scmd.prp2, & req->dma );
	
	
	vprint_request(sq,sqe,scmd,req->index);
	
	
	push_r_buf_q(req);
	
	process_read_ios_step_2();
	
	// dump stats
	///ior.raw ++ ;
	///ior_pgs.raw += req->num_lpas;
	///setReg64(stats_dev->read_IOPs , ior);
	///setReg64(stats_dev->read_IOPs_nlbs , ior_pgs);
	return;
}

void process_write_ios_step_2(){
	
	while (!w_buf_q_empty){
		
		// pick the head obj without poping it
		w_req_t *req = w_buf_q_objs[w_buf_q_head];
		
		// assign page buffers 
		if(get_write_io_buffer(req) == -1){
			return ;
		};
		// pop from pending page buffer queue
		pop_w_buf_q();
		// perform the dma
		sync_write_io_dma(req);
		
		free_write_io_buffer(req);
		// engueue completion command after dma is done
		enqueue_io_ccmd(req->cqid, (io_req_common_t*) req);
		// call buffer manager to flush wbc is needed
		//flush_all_wbc();
	
	}  
}

void process_write_io(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd){
	
	NvmeSQueue *sq = &sqs[sqid] ;
	NvmeCQueue *cq = &cqs[sq->cqid];
	w_req_t *req = (w_req_t*)sys_malloc_req_obj(WRITE_IO_REQUEST);
	
	// copy relevant fields to request data structure 
	//req->data_size = (scmd->nlbs + 1) << HOST_DS ;
	req->status = 0;
	req->sqid = sqid;
	req->cqid = cq->cqid;
	req->sqe = sqe ;
	nvmescmd_cp * to = (nvmescmd_cp*)&req->scmd;
	nvmescmd_cp * from = (nvmescmd_cp*)scmd ;
	to->cdw0_1 	= from->cdw0_1;
	to->prp1 	= from->prp1;
	to->prp2 	= from->prp2;
	to->cdw10_11 = from->cdw10_11;
	to->cdw10_11 = from->cdw10_11;
	to->cdw12_13 = from->cdw12_13;
	to->cdw14_15 = from->cdw14_15;
	
	
	if(verify_rw_param(sq, scmd) == -1){
		// post completion command
		enqueue_io_ccmd (req->cqid,(io_req_common_t*)req);
		return;
	}
	
	req->dma.host_dma_complete = false;
	req->dma.data_size = (req->scmd.nlbs+1) << HOST_DS ;
	
	// decode the prp list 
	//decode_prp((NvmeSCmd*)&req->scmd, req->data_size, &req->dma_list );
	decode_prp(req->scmd.prp1 , req->scmd.prp2, & req->dma );
	
	//lpn_t lpn = req->scmd.slba ; 
	//for ( uint32_t i =0; i <= req->scmd.nlbs ; i++ ){
	//	req->pages[i].lpn = lpn ++ ;
	//	req->pages[i].host_dma_complete = false ;
	//}
	
	vprint_request(sq,sqe,scmd,req->index);
	
	
	push_w_buf_q(req);
	
	process_write_ios_step_2();
	
	// dump stats
	///iow.raw ++ ;
	///iow_pgs.raw += req->num_lpas;
	///setReg64(stats_dev->write_IOPs , iow  );
	///setReg64(stats_dev->write_IOPs_nlbs , iow_pgs  );
}

 

void process_flush_io(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd){
	
	NvmeSQueue *sq = &sqs[sqid] ;
	NvmeCQueue *cq = &cqs[sq->cqid];
	f_req_t *req = (f_req_t*)sys_malloc_req_obj(FLUSH_IO_REQUEST);
	
	// copy relevant fields to request data structure 
	req->status = 0;
	req->sqid = sqid;
	req->cqid = cq->cqid;
	req->sqe = sqe ;
	nvmescmd_cp * to = (nvmescmd_cp*)&req->scmd;
	nvmescmd_cp * from = (nvmescmd_cp*)scmd ;
	// just copy dword 0 and 1 
	to->cdw0_1 	= from->cdw0_1;
	
	VPRINTF_NVME_IO("%s[%s] Req:%u, scmd [sqid:%u, sqe:%u, cid:%u ]" , 
					io_opcode_string[req->scmd.opcode] , request_type_string[ req->type] ,
					req->index,
					(unsigned int)sqid , 
					(unsigned int)sqe,
					(unsigned int)req->scmd.cid);
	
	//flush_all_wbc();
	
	// engueue completion
	enqueue_io_ccmd(req->cqid,(io_req_common_t*)req);
	
	sys_add_callback_ftn(check_door_ftn_id, 0 );
	
	// dump stats
	///iof.raw++;
	///setReg64(stats_dev->flush_IOPs , iof);
}

void process_dsm_io(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd){

	
	/*
	if (((NvmeDsmSCmd*)req->scmd).ad){ // trim command 
		
		VPRINTF_DISCARD("%s Cmd [sqid:%u, sqe:%u, cid:%u ]" , 
						io_opcode_string[req->opcode],
						(unsigned int)req->sq->sqid , 
						(unsigned int)req->sq->head ,
						(unsigned int)req->cid);
		
		// assign 1 page buffers 
		
		req->data_size = 4096 ;
		
		// TODO push to host dma modules
		
		
	}else{
		// post completion command 
		push(req->cq->post_queue, en_index);
		req->cq->post_queue.objs[en_index] = (void*)req ;
		post_completion_cmds(req->cq);
	}
	 */
	
	
	//sys_add_callback_ftn(check_door_ftn_id, 0 );
}

//int verify_isp_param(NvmeIOSCmd *__scmd , isp_req_t *req){

	
	//nvmescmd_cp * to = (nvmescmd_cp*)&req->scmd;
	//nvmescmd_cp * from = (nvmescmd_cp*)scmd ;
	//to->cdw12_13 	= from->cdw12_13;
	//to->cdw14_15 	= from->cdw14_15;
	//req->arg_data_part_0 = (char*)&(to->cdw12_13);
	
//	return 0;
//}
	
	

void process_isp_io(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd){
	isp_req_t *req ;
	req = (isp_req_t*) setup_io_req(sqid, sqe, scmd , ISP_IO_REQUEST);
	isp_io(scmd, req);
	sys_add_callback_ftn(check_door_ftn_id, 0 );	
}

void unknown_io_cmd(uint32_t sqid, u32 sqe, NvmeIOSCmd *scmd){
	VPRINTF_ERROR ("  *** unimplemented IO opcode:0x%x  ***" ,  (u32)scmd->opcode);
	/*req->status = NVME_SC_INVALID_OPCODE | NVME_SC_DNR ;
	// post completion command 
	push(req->cq->post_queue, en_index);
	req->cq->post_queue.objs[en_index] = (void*)req ;
	post_completion_cmds(req->cq);
	*/
	// TODO : post the ccmd
	sys_add_callback_ftn(check_door_ftn_id, 0 );	
}

void process_io_cmd(uint32_t sqid){
	
	//  copy scmd from host queue
	uint64_t host_p_addr = sqs[sqid].hostAddr ;//+ (sq->head * S_CMD_FORMAT_SIZE ) ;
	//NvmeIOSCmd * scmd = &(io_scmd_buf[sqid][sqs[sqid].head]);
	NvmeIOSCmd *scmd =(NvmeIOSCmd*) &(scmd_buf[sqs[sqid].head]);
	
	u32 sqe = sqs[sqid].head;
	// read s cmd from host queue
	sync_read_scmd(sqid, sqe , host_p_addr, (u8*)scmd , 1 );
	// increament the queue head
	inc_sq_head(&(sqs[sqid]));
	
	
	switch (scmd->opcode) {
		case NVME_OPC_READ:
			process_read_io(sqid , sqe , scmd);
			break;			
		case NVME_OPC_WRITE:
			process_write_io(sqid, sqe , scmd);
			break;
		case NVME_OPC_FLUSH:
			process_flush_io(sqid, sqe, scmd);
			break;
		case ISP_IO_OPCODE :
		case ISP_DATA_TX_OPCODE :
		case ISP_CTRL_OPCODE:
			process_isp_io(sqid, sqe , scmd);
			break;
		case NVME_OPC_DATASET_MANAGEMENT  :
			///process_dsm_io(sqid, sqe , scmd);
			///break;
			/// TODO :
		case NVME_OPC_WRITE_UNCORRECTABLE :
		case NVME_OPC_COMPARE			  :			
		default:
			unknown_io_cmd(sqid, sqe , scmd);
			break ;
	}
}

void completed_req_cbk( void * req , uint32_t type ){
	
	//VPRINTF_NVME_IO ("completed flash operation req:%u type:%u" , ((r_req_t*)req)->index, type );
	
	switch (type) {
		
		case READ_IO_REQUEST:
			sync_read_io_dma((r_req_t*)req);
			free_read_io_buffer((r_req_t*)req);
			enqueue_io_ccmd(((r_req_t*)req)->cqid, (io_req_common_t*) req);
			process_read_ios_step_2();
			process_write_ios_step_2();
			sys_add_callback_ftn(check_door_ftn_id, 0 );	
			break;
			
		case WBC_FLUSH_REQUEST:
			free_wbc_buffer((wbc_req_t*)req);
			process_write_ios_step_2();
			process_read_ios_step_2();
			sys_add_callback_ftn(check_door_ftn_id, 0 );	
			break ;
			
	
		// case others  :
			
		default:
			break;
	}
}
 
int checkSQID(uint32_t sqid){
	if (sqid){
		
		if (! sqs[sqid].sqid){ 
			VPRINTF_ERROR( " Error: DB for deleted IO SQ [ SQID:%u ]" , sqid ); 
			return 0 ;
		}
		
		if (! cqs[sqs[sqid].cqid].cqid) {
			VPRINTF_ERROR  ( " DB for deleted of CQ [ CQID , SQID:%u ] " , sqs[sqid].cqid , sqid);
			return 0 ;
		}
	}
	return 1 ;
}


void check_door_bell(uint32_t arg_0){
	
	union __attribute__ ((__packed__)) {
		reg raw ;
		dbUpdateReg dbu ;
	} newdb ;

	getReg(cmdReg->dbUpdate ,  newdb.raw )

	if(newdb.dbu.DoorBell == isSQ_DB){
		uint32_t SQIndex = (uint32_t) newdb.dbu.Qindex;
		uint32_t Tail =  (uint32_t) newdb.dbu.db_value ;
		
		VPRINTF_NVME_DB("Submission queue door bell: qid:%u, tail:%u (pre tail:%u)",
							(u32)SQIndex, (u32)Tail, (u32)sqs[SQIndex].tail );
			
		sqs[SQIndex].tail = Tail ;
			
		if (SQIndex){
			process_io_cmd(SQIndex);
		}else{
			process_admin_cmd ();
			sys_add_callback_ftn(check_door_ftn_id, 0 );	
		}
			
	}else if (newdb.dbu.DoorBell == isCQ_DB){
			
		uint32_t CQIndex = (uint32_t) newdb.dbu.Qindex;
		uint32_t Head   =  (uint32_t) newdb.dbu.db_value ;
			
		VPRINTF_NVME_DB("Completion Queue Door Bell: qid:%u, head:%u", (u32)CQIndex, (u32)Head);
			
		cqs[CQIndex].head = Head ;
		
		// check all pending completion command queue
		for ( uint32_t i = 1 ; i <= iocqCount ; i++){
			post_io_ccmds(i);
		}
	
		sys_add_callback_ftn(check_door_ftn_id, 0 );	
	} 
	else
	{
		sys_add_callback_ftn(check_door_ftn_id, 0 );
		
		idleTimeCounter++ ;
		if (idleTimeCounter >= 10000 ){
			idleTimeCounter = 0 ;
			
			// check all pending completion command queue
			for ( uint32_t i = 1 ; i <= iocqCount ; i++){
				post_io_ccmds(i);
			}


			// check status value
			union cc_register cc ;
			uint32_t shn = 0 ;

			host_int_base_reg = get_dev_address(DEV_HOST_INT);
			ctrlReg = (hostCtrlReg *)(host_int_base_reg + NVME_CTRL_OF);
			getReg(ctrlReg->cc , cc.raw )
			shn = cc.bits.shn ;
			if ( shn == 0x1 ) {
				VPRINTF_SHUTDOWN(" *** Host shutdown request ***");

				// if Gem5 simulator , send shutdown complete signal to host immediately
				// set CSTS.SHST=10b
				union csts_register csts;
				getReg(ctrlReg->csts , csts.raw ) ;
				csts.bits.shst = 0b10 ;
				setReg ( ctrlReg->csts , csts.raw ) ;

				// send shutdown signal to kernel
				sys_host_shutdown();
			}
					
		}
	}
}




