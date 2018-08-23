
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "base/chunk_generator.hh"
#include "debug/Drain.hh"
#include "sim/system.hh"

#include "iSSDHostInterface.hh"
#include "debug/iSSDHostInterface.hh"
#include "debug/iSSDHostIntCmds.hh"
#include "debug/iSSDCtrlRegAccess.hh"
#include "debug/iSSDHostRead.hh"
#include "debug/iSSDHostWrite.hh"


#define NVME_CTRL_OF 0x200

 
iSSDHostInterface::iSSDHostInterface(const Params *p):
DmaDevice(p),
pioAddr(p->pio_addr),
pioSize(p->pio_size),
pioDelay(p->pio_latency),
intNum(p->int_num),
gic(p->gic),
dmaHostWriteDelay(p->dmaHostWriteDelay),
dmaHostReadDelay(p->dmaHostReadDelay),
shmem_id(p->shmem_id)
{
	sim_verbose_fd = stdout ;// stderr ;
	print_src_line 
}

iSSDHostInterface *
iSSDHostInterfaceParams::create()
{
	return new iSSDHostInterface(this);
}

/* Determine address ranges */
AddrRangeList 
iSSDHostInterface::getAddrRanges() const
{
	AddrRangeList ranges;
	ranges.push_back(RangeSize(pioAddr, pioSize));
	return ranges;
}


/** This function allows the system to read the register entries */
Tick iSSDHostInterface::read(PacketPtr pkt)
{
	uint32_t data = 0;
	uint32_t offset ;
	uint32_t ctrl_register_offset ;

	offset = pkt->getAddr() - pioAddr ;
	
	if (pkt->getSize() != 4){
		panic(" controller register read should be 4 byte wide offset:%#x , size:%u \n" , offset , pkt->getSize() );
	}
	
	if ( offset == 4 ){ 
		data = Registers.cmdStatus ;
		
	} else if ( offset == 8 ){
	 	
	 	dbUpdateReg *dbu = deQueueDBUpdate(db_queue); 

		if( dbu ){			
			data = *((u32*)dbu);
		}
	
	}else if (offset >= NVME_CTRL_OF) {
		ctrl_register_offset = offset - NVME_CTRL_OF ;
		memcpy (&data , ((char *)nvmeReg) + ctrl_register_offset , 4 ) ;
	}
	
	pkt->set<uint32_t>(data);
	pkt->makeResponse();
	//cpu_reads++;
	return pioDelay;
}




/**  This function allows access to the writeable registers.  */
Tick iSSDHostInterface::write(PacketPtr pkt){
	
	uint32_t data ;
	unsigned int offset ;
	unsigned int ctrl_register_offset ;

	print_src_line
	offset = pkt->getAddr() - pioAddr ;

	if (pkt->getSize() != 4){
		panic(" controller register write should be 4 byte wide offset:%#x , size:%u \n" , offset , pkt->getSize() );
	}

	data = pkt->get<uint32_t>() ;

	if (offset == 0){

		Registers.cmd = data ;
		switch (Registers.cmd) {
				
			case INIT :
				// Initialize host interface 
				Registers.cmdStatus = PROCESSING ;print_src_line
				connect_shared_memory();print_src_line
				Registers.cmdStatus = INITCOMPLETE ;
				break;

			case HOST_CONNECTION_STATUS:
				//printf("%s\n","check connection %x" , sim_meta->simulation_status);
				// Get Host connection status:
				if (sim_meta->simulation_status == 200){
					Registers.cmdStatus = HOST_ATTACHED ;
				}else{
					Registers.cmdStatus = HOST_DETTACHED ;
				}
				break ;

			case PCI_DMA_READ:
			case PCI_DMA_SQ_READ:
				start_read_host();
				break;
				
			case PCI_DMA_WRITE:
			case PCI_DMA_CQ_WRITE:	
				start_write_host () ;
				break;
				
			case PCI_ENABLE_MSIX_VECTOR :
				enable_msix_on_hostcpu() ;
				break ;
				
			default :
				panic ("illegal command from firmware");
				break;
				
		}

	} 

	else if (offset < sizeof(hostCmdReg) ){print_src_line
		
		//DPRINTF(iSSDHostIntCmds, 
		//fprintf(sim_verbose_fd, 	" write command register [offset:%#x, data: %#x (%u)] \n" , offset , data , data  ) ;
		memcpy( ((char *)&Registers) + offset , &data , pkt->getSize());
		
	}
	else if (offset >= NVME_CTRL_OF){print_src_line
		
		ctrl_register_offset = offset - NVME_CTRL_OF ;
		
		if ( ctrl_register_offset  < 0x1000) {print_src_line
			//DPRINTF(iSSDCtrlRegAccess, 
			//fprintf(sim_verbose_fd, " write nvme controller register [offset:%#x, data: %#x (%u) ] \n" , ctrl_register_offset , data , data ) ;	
			memcpy( ((char *)nvmeReg) + ctrl_register_offset , &data , pkt->getSize());
		}else{print_src_line
			panic("illegal nvme controller register offset \n") ;
		}
		
	} else {print_src_line
		 panic(" illegal write offset \n");
	}
	print_src_line
	////cpu_writes ++ ;
	pkt->makeResponse();
	return pioDelay;
}


void iSSDHostInterface::connect_shared_memory(){print_src_line

	//FILE    *shmem_id_fd ;
    //char    shmem_id[1<<10] ;
    //char    shmem_id_file [1024] = "sim_out/shared_memory_ids.ini" ;
    int     shmem ;
    void*   	shmem_ptr ;
    u32     shmem_size ;
    print_src_line
    shmem_size = sizeof(struct SharedMemoryLayout);

    fprintf(sim_verbose_fd, " Shared memory : %s \n", shmem_id.c_str()  );

    if(((shmem = shm_open(  shmem_id.c_str()   , O_RDWR | O_CREAT  , 0666 ))>=0)){
               
             if((ftruncate(shmem , shmem_size )) == -1){
                fprintf(sim_verbose_fd , "[%s] ftruncate error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
                fflush(sim_verbose_fd);
             }        
                 
             shmem_ptr =  mmap(0, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmem, 0);
             if (! shmem_ptr ){
                fprintf(sim_verbose_fd , "[%s] mmap error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
                fflush(sim_verbose_fd);
             }
    }else{
       fprintf(sim_verbose_fd , "[%s] shmem error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
       fflush(sim_verbose_fd);
    }

    memset (shmem_ptr  , 0 , sizeof(shmem_layout_t));
     
    fprintf(sim_verbose_fd , "[%s] shared memory attached successfuly :%d, ptr:%p\n",__FUNCTION__,shmem, shmem_ptr);


     
    shared_mem = ( shmem_layout_t *)shmem_ptr ;
    sim_meta =  & (shared_mem->sim_meta) ;
    db_queue =  & (shared_mem->db_queue) ;
    msg_queue =  & (shared_mem->msg_queue );
    page_queue =  & (shared_mem->page_queue) ;
    nvmeReg = & (shared_mem->nvmeReg ); 	//nvmeRegPtr = nvmeReg ;	

    // init embedded queues 
    db_queue->head = db_queue->tail = 0;
    msg_queue->head = msg_queue->tail = 0;
    page_queue->head = page_queue->tail = 0 ;

    //if (!sim_meta->host_started){
    //	panic (" Host is not ready ");
    //}

    strcpy((char*)sim_meta->device_name , "GEM5 Simulator") ;
    //fprintf(sim_verbose_fd, " Host Name %s  state:%lu \n", sim_meta->host_name , sim_meta->simulation_status  ); fflush(sim_verbose_fd);

    sim_meta->device_started = 1 ;

    sim_meta->simulation_status = 100 ;

    print_src_line
}

// Read Host Memory Steps  

void iSSDHostInterface::start_read_host(){print_src_line
	
	// get a page register from within the shared memory
	page_reg_t* page = enQueuePageRegister(page_queue);
	if (! page){
		panic ("Shared memory page register full ");
	}

	// get a msg slot 
	ssd_msg_t * msg = enQueueSSDMsg(msg_queue , Registers.cmd );
	if(!msg){
		panic ("shared memory msg queue full ") ;
	}

	// send msg to host 
	if (msg->registers.cmd == PCI_DMA_READ){
		
		msg->registers.dma_addr.raw = Registers.dma_addr.raw ;
		msg->registers.ssdAddr = Registers.ssdAddr ;
		msg->registers.size = Registers.size ;
		
		//DPRINTF(iSSDHostRead, "PCI_DMA_READ(SQ): [dma_addr:%#llx, bytes:%d, ssdAddr:%#x  ] \n", 
		//		msg->registers.dma_addr.raw, msg->registers.size, msg->registers.ssdAddr) ;
		
	}else if ( msg->registers.cmd == PCI_DMA_SQ_READ ){
		
		msg->registers.dma_addr.raw = Registers.sqeDmaAddr.raw + ( Registers.sqe * 64 );	
		msg->registers.ssdAddr = Registers.sqAddr ;
		msg->registers.size = 64 ;
		msg->registers.squeue = Registers.squeue;
		msg->registers.sqe = Registers.sqe;
		 
		//DPRINTF(iSSDHostRead, "PCI_DMA_SQ_READ: [sqIndex:%u, head:%u, ssdAddr:%#x ] \n", 
		//				Registers.squeue, Registers.sqe , Registers.sqAddr) ;

	}
	else{ panic (" %s:%u " , __FUNCTION__ , __LINE__ ) ; } 

	msg->page_index = page->page_index ;
	msg->msg_ready = true ;
	Registers.cmdStatus = PROCESSING ;
	
	Event_read_host_complete *callBack = new Event_read_host_complete(this , msg );
	schedule(callBack , curTick() + dmaHostReadDelay );
print_src_line
}

void iSSDHostInterface::read_host_complete(ssd_msg_t *msg){print_src_line
		
	page_reg_t* page ;
		
	page = getPageRegister(page_queue , msg->page_index );

	if (! page ){
		panic (" page index out of range ") ;
	}

	//int loop_limit = 100000 ;
	while(1){
	//for ( int i = 0 ; i < loop_limit ; i++){
		if(page->data_status == data_status_ready ) break ;
	}

	if(page->data_status != data_status_ready ){
		panic (" host not responding ");
	}


	//DPRINTF(iSSDHostRead, "    read host complete \n") ;
	
		
	// move data to SSD Memory
	Event_write_ssd_complete *callback = new Event_write_ssd_complete(this, msg ); 
	dmaWrite( msg->registers.ssdAddr, msg->registers.size , callback , (u8*)& page->data[0]);
	//}
print_src_line
}

void iSSDHostInterface::write_ssd_complete(ssd_msg_t *msg){print_src_line

	page_reg_t* page ;
	
	DPRINTF(iSSDHostRead, "    COMPLETE  \n") ;

	page = getPageRegister(page_queue , msg->page_index );
	if (! page ) panic (" page index out of range ") ;
	page->data_status = data_status_used ;

	Registers.cmdStatus = COMPLETE ;	
print_src_line
}

// End Read Host routine



// Write Host Routine

void iSSDHostInterface::start_write_host(){print_src_line

	// get a page register from within the shared memory
	page_reg_t* page = enQueuePageRegister(page_queue);
	if (! page){
		panic ("Shared memory page register full ");
	}

	// get a msg slot 
	ssd_msg_t * msg = enQueueSSDMsg(msg_queue , Registers.cmd );
	if(!msg){
		panic ("shared memory msg queue full ") ;
	}

	// send msg to host 
	if (msg->registers.cmd == PCI_DMA_WRITE){ 
	
		msg->registers.dma_addr.raw = Registers.dma_addr.raw ;
		msg->registers.ssdAddr = Registers.ssdAddr ;
		msg->registers.size = Registers.size ;
		
		//DPRINTF(iSSDHostWrite, "PCI_DMA_WRITE(CQ): [dma_addr:%#llx, bytes:%d, ssdAddr:%#x , buffMgtID:%u] \n", 
		//		Registers.dma_addr.raw, Registers.size, Registers.ssdAddr , Registers.buffMgtID) ;

	}else if (msg->registers.cmd == PCI_DMA_CQ_WRITE){
	
		msg->registers.dma_addr.raw = Registers.cqeDmaAddr.raw  + ( Registers.cqe * 16 );
		msg->registers.ssdAddr = Registers.cqAddr ;
		msg->registers.size = 16 ;
		msg->registers.cqe = Registers.cqe ;
		msg->registers.cqueue = Registers.cqueue ;
		msg->registers.cqIntVector = Registers.cqIntVector ;
		msg->registers.isr_notify = Registers.isr_notify ;

		//DPRINTF(iSSDHostWrite, "PCI_DMA_CQ_WRITE: [cq index:%u, tail:%u, ssdAddr:%#x , isr:%u,%u ] \n", 
		//		Registers.cqueue, Registers.cqe, Registers.cqAddr, Registers.isr_notify, Registers.cqIntVector) ;
	}
	else { panic (" %s:%u " , __FUNCTION__ , __LINE__ ) ; }
 
	msg->page_index = page->page_index ;
	msg->msg_ready = true ;
			
				
	Registers.cmdStatus = PROCESSING ;
	

	Event_read_ssd_complete   *callback = new Event_read_ssd_complete  (this, msg ); 
	dmaRead(msg->registers.ssdAddr, msg->registers.size , callback  , (u8*)&page->data[0] );
	
print_src_line
}

void iSSDHostInterface::read_ssd_complete(ssd_msg_t *msg){print_src_line

	page_reg_t* page ;
		
	page = getPageRegister(page_queue , msg->page_index );

	page->data_status = data_status_ready ;

	Event_write_host_complete *callBack = new Event_write_host_complete (this , msg );
	schedule(callBack , curTick() + dmaHostReadDelay );
}

void iSSDHostInterface::write_host_complete(ssd_msg_t *msg){print_src_line
	
	DPRINTF(iSSDHostWrite, "    COMPLETE  \n") ;
	
	page_reg_t* page ;
		
	page = getPageRegister(page_queue , msg->page_index );


	while(1){
		if (page->data_status == data_status_used)break ;
	}

	Registers.cmdStatus = COMPLETE ;
print_src_line
}

// End Write Host Routine

 
void iSSDHostInterface::enable_msix_on_hostcpu(){print_src_line

	// get a msg slot 
	ssd_msg_t * msg = enQueueSSDMsg(msg_queue , Registers.cmd );
	if(!msg){
		panic ("shared memory msg queue full ") ;
	}

	msg->registers.cqIntVector =  Registers.cqIntVector ;
	msg->msg_ready = true ;

	Registers.cmdStatus = COMPLETE ;	
}

	

/* stats  */
//void
//iSSDHostInterface::regStats(){
//	
//	
//	
//	using namespace Stats;
//	
//	cpu_reads
//	.name(name() + ".cpu_reads")
//	.desc("Number of read request from CPU")
//	.flags(total)
//	;
//	
//	
//	cpu_writes
//	.name(name() + ".cpu_writes")
//	.desc("Number of write request from CPU")
//	.flags(total)
//	;
//	
//}



