/*
 * File: iSSDNandCtrl.cc
 * Date: 2017. 05. 05.
 * Author: Thomas Haywood Dadzie (ekowhaywood@hanyang.ac.kr)
 * Copyright(c)2017 
 * Hanyang University, Seoul, Korea
 * Security And Privacy Laboratory. All right reserved
 */


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

#include "debug/iSSDNandCtrl.hh" 
#include "debug/iSSDNandCtrlOP.hh"
#include "iSSDNandCtrl.hh"
  

iSSDNandCtrl::iSSDNandCtrl(const Params *p):
DmaDevice(p),
pioAddr(p->pio_addr),
pioSize(p->pio_size),
pioDelay(p->pio_latency),
intNum(p->int_num),
gic(p->gic),

numChannel ( p->numChannel ),
numPackages ( p-> numPackages),
DS ( p->DS  ),
numDies (p->numDies ),
numPlanes ( p->numPlanes ),
numBlocks ( p->numBlocks ),
numPages  ( p->numPages),

media_type (p->media_type),

ltcy_rcmd_issue ( p->nand_ltcy_rcmd_issue ) ,
ltcy_pcmd_issue ( p->nand_ltcy_pcmd_issue ) ,
ltcy_ecmd_issue ( p->nand_ltcy_ecmd_issue ) ,
ltcy_read_page ( p->nand_ltcy_read_page ) ,
ltcy_program_page ( p->nand_ltcy_program_page ) ,
ltcy_erase_block ( p->nand_ltcy_erase_block ) ,
nand_ltcy_read_msb_page ( p->nand_ltcy_read_msb_page),
nand_ltcy_read_lsb_page ( p->nand_ltcy_read_lsb_page),
nand_ltcy_program_msb_page ( p->nand_ltcy_program_msb_page),
nand_ltcy_program_lsb_page ( p->nand_ltcy_program_lsb_page),
nand_ltcy_read_csb_page ( p->nand_ltcy_read_csb_page),
nand_ltcy_program_csb_page ( p->nand_ltcy_program_csb_page),
nand_ltcy_read_chsb_page ( p->nand_ltcy_read_chsb_page),
nand_ltcy_read_clsb_page ( p->nand_ltcy_read_clsb_page),
nand_ltcy_program_chsb_page ( p->nand_ltcy_program_chsb_page),
nand_ltcy_program_clsb_page ( p->nand_ltcy_program_clsb_page),
NC_index(p->index_num),
chnBufferSize ( p->chnBufferSize ),
dmaBufferSize(p->dmaBufferSize),
imgFile(p->imgFile)
{
	//dmaBuffer = (unsigned char *)malloc(dmaBufferSize);
	DPRINTF(iSSDNandCtrl, "\n %s nand  created  pioAddr:%p \n" , media_type.c_str() ,  (void *)pioAddr  ) ;
	
	channel_0 = new NC_Channel( p , numPackages * numDies * numPlanes , this , p->chn_0_index );
	channel_1 = new NC_Channel( p , numPackages * numDies * numPlanes , this , p->chn_1_index );
	dma_engine = new NC_Dma_Engine(this);
	
	if (!(media_type == "slc" || media_type == "mlc" || media_type == "tlc" || media_type == "qlc")){
		panic ("Nand media type not set ");
	}
}

bool usingShmMem = false ;
bool img_file_loaded = false ;
uint64_t img_file_address = 0 ;
uint64_t img_file_size;
 
void * iSSDNandCtrl::loadImgProcess(){
	
	struct stat fileInfo;
	FILE* sim_verbose_fd = stdout ;// stderr ;
		    

	if (img_file_address == 0 && !(img_file_loaded)){	

		int fd = open( imgFile.c_str()  , O_RDWR);
		if (fd == -1){
			DPRINTF(iSSDNandCtrl, " Error opening file for writing \"%s\" \n" ,  imgFile.c_str());
			fprintf(stderr , " Error opening file for writing \"%s\" \n" ,  imgFile.c_str());
			panic("Error opening file for writing");
			
		}        
		
		if (fstat(fd, &fileInfo) == -1){
			panic("Error getting the file size");
			
		}
		
		if (fileInfo.st_size == 0){
			DPRINTF(iSSDNandCtrl, " Error: File is empty  \n" );
			fprintf(stderr, " Error: File is empty  \n" );
		}
		
		char *map = (char *)mmap(0, fileInfo.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);
		if (map == MAP_FAILED){
			close(fd);
			panic("Error mmapping the file");
		}
		
		imgSize =  fileInfo.st_size ;
		imgPtr  = (uint64_t) map ;
		
		fprintf(stderr, "\n\n imgFile:%s , Size:%lu MB ( %lu Bytes), imgPtr:%#lx  " , imgFile.c_str()  , (imgSize >> 20), imgSize ,  imgPtr);
		{

			FILE    *shmem_id_fd ;
		    char    shmem_id[1024] ;
		    char    shmem_id_file [1024] = "sim_out/img_shared_memory.ini" ;
		    int     shmem ;
		    u8*     shmem_ptr ;
		    size_t  shmem_size ;

		    shmem_ptr = NULL ;
			
			// Attempt caching image in shared memory 
			fprintf(stderr, "\n Attempt caching image in shared memory  " );

	    	shmem_size = imgSize ; // sizeof(struct SharedMemoryLayout);

	    	fprintf(sim_verbose_fd, " Reading shared memory name from %s file \n", shmem_id_file );fflush(sim_verbose_fd);
	    
	    	if (! (shmem_id_fd = fopen (shmem_id_file , "r"))){
	    	    fprintf(sim_verbose_fd, "Error openning shared memory ID file {%s} \n", shmem_id_file );fflush(sim_verbose_fd)  ; 
	    	    exit(-1); 
	    	}

	    	int len = fscanf (shmem_id_fd , "%s" , shmem_id ) ;

	    	fprintf(sim_verbose_fd, " Shared memory name : [%d]%s\n",  len , shmem_id  );
		    fflush(sim_verbose_fd);

		    if(((shmem = shm_open( shmem_id  , O_RDWR | O_CREAT  , 0666 ))>=0)){
		               
	             if((ftruncate(shmem , shmem_size )) == -1){
	                fprintf(sim_verbose_fd , "[%s] ftruncate error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
	                fflush(sim_verbose_fd);
	             }        
	                 
	             shmem_ptr = (u8*)mmap(0, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmem, 0);
	             if (! shmem_ptr ){
	                fprintf(sim_verbose_fd , "[%s] mmap error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
	                fflush(sim_verbose_fd);
	             }

	            fprintf(sim_verbose_fd , "[%s] shared memory attached successfuly :%d, ptr:%p size:%lu \n",__FUNCTION__,shmem, shmem_ptr , shmem_size );
			    fflush(sim_verbose_fd);

	            
				usingShmMem =true;
				memcpy(shmem_ptr, (void *)imgPtr , imgSize );
				//fprintf (sim_verbose_fd , " , successfully cached to shmid:%d  [@ %#lx] \n" ,shmid , (uint64_t)shm );
				imgPtr = (uint64_t) shmem_ptr  ;
				
				//munmap(map, fileInfo.st_size  );
				close(fd);
				

		    }else{
		       fprintf(sim_verbose_fd , "[%s] shmem error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem , errno, strerror(errno));
		       fflush(sim_verbose_fd);
		    }
		     
		    
		}

		img_file_address = imgPtr ;
		img_file_size = imgSize ;
		img_file_loaded = true ;
	}
	else{
		fprintf(sim_verbose_fd, "\n image pre-loaded  ptr:%#lx , size:%lu \n", img_file_address , img_file_size );
		imgPtr = img_file_address;
		imgSize = img_file_size;
	}



	/*   create channel instances */
	//if (DS < 9 || DS > 14) DS = 12 ;
	
	config = (NAND_CTRL_CONFIG * )malloc(sizeof(NAND_CTRL_CONFIG  ));
	
	imgPages = imgSize >> DS  ;
	imgPagesMapped = 0 ;
	
	
	//fprintf( stderr ,	"  Number of Channels :%u  \n" , numChannel );
	//fprintf( stderr ,	"   ****   Channel Configurations   **** \n");
	//fprintf( stderr ,	"   chnID |  packages  |  dies  |  Planes |   Blocks  |  Pages  | DS  |  PageRange \n");
	
	config->numChn = numChannel ;
	
	//for(uint32_t ix = 0 ; ix < numChannel ; ix++){ 
		
		//uint32_t planes_per_chn = numPackages * numDies * numPlanes ;
		//uint32_t pages_per_chn = planes_per_chn * numBlocks * numPages ;
		//uint32_t pages_per_plane = numBlocks * numPages ;
		
		//uint64_t chnsPage = imgPagesMapped ;
		/*  map planes */
		//for (uint32_t iplane = 0 ; iplane < planes_per_chn ; iplane ++ ){
		//	imgPagesMapped += pages_per_plane ;
		//}
		
		//uint64_t chnePage = imgPagesMapped - 1 ;
		
		// update config data structure  
//		config->chnConfig[ix].chnBufferSize = 0 ;//chnBufferSize ;
//		config->chnConfig[ix].pkgs = numPackages ;
//		config->chnConfig[ix].dies =  numDies;
//		config->chnConfig[ix].ds = DS ;
//		config->chnConfig[ix].planes = numPlanes ;
//		config->chnConfig[ix].planeblks = numBlocks ;
//		config->chnConfig[ix].blockpgs = numPages ;  
		//uint64_t dsize = 1 << config->chnConfig[ix].ds ;
		//uint64_t bf = config->chnConfig[ix].chnBufferSize / 1024 ;
//		fprintf (DPRINT_FILE , "    %u   %u  %u   %u    %u    %u  %u  [%lu ~ %lu] \n" , ix ,
//				 (uint32_t)config->chnConfig[ix].pkgs , 
//				 (uint32_t)config->chnConfig[ix].dies , 
//				 config->chnConfig[ix].planes ,
//				 config->chnConfig[ix].planeblks, 
//				 config->chnConfig[ix].blockpgs , 
//				 (uint32_t)config->chnConfig[ix].ds 
//				 , chnsPage , chnePage 
//				 );
			
		
	//}
	
	//numPagesPerChannel = imgPagesMapped / numChannel ;
	 
	//fprintf(DPRINT_FILE , "\n  mapping complete PagePerChannel:%u , unmapped imp pages %u \n" , numPagesPerChannel , (imgPages - imgPagesMapped) ); 
	//fflush(DPRINT_FILE); fflush(DPRINT_FILE);
	
	return (void*)0;
}

/**
 * Determine address ranges
 */
AddrRangeList iSSDNandCtrl::getAddrRanges() const
{
	AddrRangeList ranges;
	ranges.push_back(RangeSize(pioAddr, pioSize));
	return ranges;
}


/*
 *  This function allows the system to read the
 *  register entries
 */
Tick iSSDNandCtrl::read(PacketPtr pkt)
{
	uint32_t data = 0;
	unsigned int offset = pkt->getAddr() - pioAddr ;	
	unsigned int offset_2 = 0;
	NC_Channel *channel = (NC_Channel*)0;
	//printf ( "  read nand register: offset:%#x , size:%u  \n" , offset ,pkt->getSize()  ) ; //fflush(stdout);
	
	if (offset >= CHANNEL_1_BASE_ADDR ){
		channel = channel_1;
		offset_2 = offset - CHANNEL_1_BASE_ADDR ;
	}else if (offset >= CHANNEL_0_BASE_ADDR ){
		channel = channel_0;
		offset_2 = offset - CHANNEL_0_BASE_ADDR;
	}
	else if (offset < sizeof(Registers) ){ 
	
		memcpy(&data , ((char *)&Registers) + offset , pkt->getSize());
		
		if ( offset != 0x4 ){
			DPRINTF(iSSDNandCtrl, "  read nand register: offset:%#x , size:%u , data:%x (%u) \n" , 
					offset ,pkt->getSize() , data , data ) ;
		}
		pkt->set<uint32_t>(data);
	}
	
	 	
	if(channel){
		
		if (offset_2 == CHIP_STATUS){
			//printf ( "\nchannel %u , read chip status : %u", channel->chn_index , data);
			pkt->set<uint32_t>(data);
		}
		else if (offset_2 ==  WAITING_ROOM_CMD_COUNT)
		{
			data = channel->cmd_waiting_room_size();
			//fprintf (stdout, "\nchannel %u , read waiting room cmd count : %u", channel->chn_index , data);
			//fflush(stdout);
			pkt->set<uint32_t>(data);
		}
		else if (offset_2 == ACTIVE_OPERATION_COUNT){
			data = channel->active_nand_ops + channel->cmd_waiting_room_size() ;
			//fprintf(stdout , "\nchannel %u , read active operation count : %#x ", 
			//		channel->chn_index , data );
			//fflush(stdout);
			pkt->set<uint32_t>(data);
		}
	}
	
	cpu_reads++;
	pkt->makeResponse();
	return pioDelay;
}

/*
 * This function allows access to the writeable
 * registers. 
 */
Tick iSSDNandCtrl::write(PacketPtr pkt)
{
	uint32_t data = 0;
	unsigned int offset = pkt->getAddr() - pioAddr ;	
	unsigned int offset_2 = 0;
	NC_Channel *channel = (NC_Channel*)0;	
	//fprintf ( stdout, "\n NC:%u write nand register: offset:%#x , size:%u  " , NC_index , offset ,pkt->getSize()  ) ; //fflush(stdout);
	
	if (offset >= CHANNEL_1_BASE_ADDR ){
		channel = channel_1;
		offset_2 = offset - CHANNEL_1_BASE_ADDR ;
	}
	else if (offset >= CHANNEL_0_BASE_ADDR ){
		channel = channel_0;
		offset_2 = offset - CHANNEL_0_BASE_ADDR;
	}
	else if ( offset == 0){
		data  = pkt->get<uint32_t>() ;
		switch (data) 
		{
				case INIT_NAND_CTRL:
					Registers.cmdStatus = PROCESSING;
					loadImgProcess();
					Registers.cmdStatus = COMPLETE;
				break;
				
				case NAND_CTRL_SHUTDOWN:
					Registers.cmdStatus = PROCESSING;
					shutdown();
				break;
				
			default: // unknown command
				break;
		}
	}
	
	
	if(channel){
	
		if (offset_2 == CHIP_SELECT_REG){
			data = pkt->get<uint32_t>() ;
			if ( data > channel->chip_count ){
				panic ( " channel %u , chip select is out of range , data:%u", channel->chn_index , data );
			}
			channel->new_cmd->chip_enabled = channel->chips[data];
			//fprintf (stdout , "\n channel %u , write chip select : %u , chip->index:%u",channel->chn_index , data ,
			//		 channel->new_cmd->chip_enabled->chip_index );
			//fflush(stdout);
		}
		else if ( offset_2 == ADDRESS_CYCLE_REG_1 ){
			data = pkt->get<uint32_t>() ;
			channel->new_cmd->address_cycle.addr.addr_1 = data ;
			//fprintf ( stdout ,"\n channel %u , write address 1 : %#x",channel->chn_index , data);
			//fflush(stdout);
		}
		else if (offset_2 == ADDRESS_CYCLE_REG_2){
			data = pkt->get<uint32_t>() ;
			channel->new_cmd->address_cycle.addr.addr_2 = data ;
			//fprintf (stdout , "\n channel %u , write address 2 : %#x",channel->chn_index , data);
			//fflush(stdout);
		}
		else if (offset_2 == DMA_RAM_PTR){
			data =  pkt->get<uint32_t>() ;
			channel->new_cmd->ram_ptr = data ;
			//fprintf ( stdout , "\n channel %u , write ram ptr : %#x",channel->chn_index , data);
			//fflush(stdout);
		}
		else if (offset_2 == CMD_PHASE_CMD_REG){
			data = pkt->get<uint32_t>();
			channel->new_cmd->cmd.data = data ;
			//fprintf ( stdout ,"\n channel %u , write cmd phase : %x",channel->chn_index , data);
			//fflush(stdout);
		
			bool cmd_room_is_empty = channel->cmd_waiting_room->empty() ;
			channel->add_cmd_phase(channel->new_cmd);
			if (cmd_room_is_empty ){ 
				// call the process cmd ftn if the queue was empty 
				process_waiting_room_cmd(channel);
			}
			channel->new_cmd = new NC_Command();
		}
		
		else if (offset_2 == DMA_PHASE_CMD_REG){
			data = pkt->get<uint32_t>();
			channel->new_cmd->cmd.data = data; 
			//fprintf ( stdout , "\n channel %u , write data phase : %x", channel->chn_index ,data);
			//fflush(stdout);
			
			bool cmd_room_is_empty = channel->cmd_waiting_room->empty() ;
			channel->add_data_phase(channel->new_cmd);
			if (cmd_room_is_empty ){ 
				// call the process cmd ftn if the queue was empty 
				process_waiting_room_cmd(channel);
			}
			channel->new_cmd = new NC_Command();
		}
	}
	
	cpu_writes ++ ;
	pkt->makeResponse();
	return pioDelay;
}
     


iSSDNandCtrl * iSSDNandCtrlParams::create()
{
	return new iSSDNandCtrl(this);
}
 




 
				
iSSDNandCtrl:: PageDataPacket * 
iSSDNandCtrl::ImageManager_getData()// uint32_t page , NandPlane * plane , Channel * chn )
{

//	//uint32_t DS = 9 ; //TODO 
//	//uint32_t pSize = chn->pageSize ;
//	//NandPage *pg = chnop->spage ;
//	//uint8_t * dst = chn->channelBuffer ;
//	
//	iSSDNandCtrl:: PageDataPacket * pdpkt = new iSSDNandCtrl:: PageDataPacket( chn->getpageSize()  );
//	
//	DPRINTF2(iSSDNandCtrl , " getImgPage ( page:%#llu , onPlane:%llu , dataSize:%llu  ) \n" ,
//			 page , plane->getID() , pdpkt->getSize() ); 
//	
//	
//	//uint64_t pgs =  npbs + 1;
//	uint64_t pgoffset = page << chn->getDS() ;
//	//uint64_t data_size = pgs << DS  ;
//	//uint64_t src = imgPtr + pgoffset ;
//
//	pdpkt->setData( (uint8_t *)	(imgPtr + pgoffset));
//
//	
//	//memcpy(dst ,  (uint8_t *)src ,  data_size );
	return 0;// pdpkt ;
}

				
				
uint64_t 
iSSDNandCtrl::ImageManager_setData()// PageDataPacket *src_pdpkt , uint32_t page , NandPlane * plane , Channel * chn )
{
	
//	//uint32_t DS = 9 ; //TODO 
//	//uint32_t pSize = chn->pageSize ;
//	//NandPage *pg = chnop->spage ;
//	//uint8_t * dst = chn->channelBuffer ;
//	
//	DPRINTF2(iSSDNandCtrl , " setImgPage ( page:%#llu , onPlane:%llu , dataSize:%llu  ) \n" ,
//			 page , plane->getID() , src_pdpkt->getSize() );
//	
//	
//	//uint64_t pgs = npbs + 1 ;
//	uint64_t pgoffset = page << chn->getDS() ;
//	//uint64_t dst = imgPtr + pgoffset ;
//	//uint64_t data_size = pgs << DS  ;
//	
//	src_pdpkt->getData((uint8_t *)	(imgPtr + pgoffset) ) ;
//	
//	
//	//DPRINTF(iSSDNandCtrl , " setImgPages ( imgOffset:%llu , src:%#llx, dataSize:%llu  ) \n" , pgoffset , src , data_size); 
//	
//	//memcpy((uint8_t *)dst ,  src ,  data_size );
	
	return  0;//src_pdpkt->getSize()   ;
}
		

void iSSDNandCtrl::shutdown(){
	
	if (usingShmMem){
	
		fprintf (stderr , "\n  start writing shm data to file %s , size:%lu " ,  imgFile.c_str() , imgSize );
		fflush(stderr);
		
		struct stat fileInfo;
		
		int fd = open( imgFile.c_str()  , O_RDWR);
		if (fd == -1){
			DPRINTF(iSSDNandCtrl, " Error opening file for writing \"%s\" \n" ,  imgFile.c_str());
			fprintf(stderr , " Error opening file for writing \"%s\" \n" ,  imgFile.c_str());
			panic("Error opening file for writing");
			
		}        
		
		if (fstat(fd, &fileInfo) == -1){
			panic("Error getting the file size");
			
		}
		
		if (fileInfo.st_size == 0){
			DPRINTF(iSSDNandCtrl, " Error: File is empty  \n" );
			fprintf(stderr, " Error: File is empty  \n" );
		}
		
		char *map = (char *)mmap(0, fileInfo.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);
		if (map == MAP_FAILED){
			close(fd);
			panic("Error mmapping the file");
		}
		
		
		memcpy(map , (void *)imgPtr , imgSize );
		fprintf (stderr , " , successfully \n" );
		
		munmap(map, fileInfo.st_size );
		close(fd);
	}
	
	Registers.cmdStatus = COMPLETE ;
	
}


void
iSSDNandCtrl::regStats(){
	using namespace Stats;
	
	cpu_reads
	.name(name() + ".cpu_reads")
	.desc("Number of read request from CPU")
	.flags(total)
	;
	
	cpu_writes
	.name(name() + ".cpu_writes")
	.desc("Number of write request from CPU")
	.flags(total)
	;
	
}


Tick iSSDNandCtrl::NC_Chip::get_read_ltcy( uint32_t page_index ){
	
	if (parent->media_type == "slc" ){
		return parent->ltcy_read_page;
	}
	else if (parent->media_type == "mlc"){
		
		int page_type = page_index % 2 ;
		
		if (page_type){ // MSB page
			return parent->nand_ltcy_read_msb_page;
		}
		else{ // LSB page
			return parent->nand_ltcy_read_lsb_page ; 
		}
	}
	else if (parent->media_type == "tlc"){
		
		int page_type = page_index % 3 ;

		if (!page_type){
			return parent->nand_ltcy_read_lsb_page ; 
		}
		else if (page_type == 1) {
			return parent->nand_ltcy_read_csb_page ;
		}
		else if (page_type == 2){
			return parent->nand_ltcy_read_msb_page ; 
		}
	}
	else if (parent->media_type == "qlc"){
		
		int page_type = page_index % 4 ;
		
		if (!page_type){
			return parent->nand_ltcy_read_lsb_page ; 
		}
		else if (page_type == 1) {
			return parent->nand_ltcy_read_clsb_page ;
		}
		else if (page_type == 2){
			return parent->nand_ltcy_read_chsb_page ;
		}
		else if (page_type == 3){
			return parent->nand_ltcy_read_msb_page ; 
		}
	}
	
	return 0;
}

Tick iSSDNandCtrl::NC_Chip::get_program_ltcy(uint32_t page_index ){
	
	if (parent->media_type == "slc" ){
		return parent->ltcy_program_page;
	}
	
	else if (parent->media_type == "mlc"){
		
		int page_type = page_index % 2 ;
		
		if (page_type){ // MSB page
			return parent->nand_ltcy_program_msb_page;
		}
		else{ // LSB page
			return parent->nand_ltcy_program_lsb_page ; 
		}
	}
	else if (parent->media_type == "tlc"){
		
		int page_type = page_index % 3 ;
		
		if (!page_type){
			return parent->nand_ltcy_program_lsb_page ; 
		}
		else if (page_type == 1) {
			return parent->nand_ltcy_program_csb_page ;
		}
		else if (page_type == 2){
			return parent->nand_ltcy_program_msb_page ; 
		}
	}
	else if (parent->media_type == "qlc"){
		
		int page_type = page_index % 4 ;
		
		if (!page_type){
			return parent->nand_ltcy_program_lsb_page ; 
		}
		else if (page_type == 1) {
			return parent->nand_ltcy_program_clsb_page ;
		}
		else if (page_type == 2){
			return parent->nand_ltcy_program_chsb_page ;
		}
		else if (page_type == 3){
			return parent->nand_ltcy_program_msb_page ; 
		}
	}
		
	return 0 ;	
}

