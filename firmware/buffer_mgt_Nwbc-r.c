 

#define N_Caches 8
#define N_WBCaches 8

unsigned char * page_buffer[N_Caches] ;
bool page_buffer_inuse[N_Caches] ;

wbc_req_t wbc[N_WBCaches];
bool wbc_inuse[N_WBCaches];

int init_buffer_mgt(){
	
	for(u32 i = 0 ; i < N_Caches ; i++){
		page_buffer_inuse[i] = false ;
		page_buffer[i] = get_uncacheable_ram(MAX_TX_SIZE);
		VPRINTF_BUFF_MGT("%u Page Buffer : buffer addr:0x%x Size:%u KB" ,
							  i+1, page_buffer, (MAX_TX_SIZE) >> 10);
	}
	
	for (u32 i = 0 ; i < N_WBCaches ; i++){
		wbc[i].index = i+1;
		wbc[i].type = WBC_FLUSH_REQUEST ;
		wbc_inuse[i] = false;
	
		for (uint32_t icache = 0 ; icache < WBC_PAGES ; icache++){
			wbc.pages[icache].valid = 0;
		}
	}
	
	return 0;
}

int get_read_io_buffer (r_req_t *req ){	
	
	u32 free_index = 0;

	for(u32 i = 0 ; i < N_Caches ; i++){
		if (page_buffer_inuse[i] == false){
			free_index = i+1;
			break;
		}
	}
	if (!free_index) return -1;

	req->dma.buffer_addr = page_buffer[free_index-1] ;
	VPRINTF_BUFF_MGT(" read io page buffer req:%u " , req->index);
	for ( uint16_t i =0 ; i <= req->scmd.nlbs ; i++ ){
		req->dma.list[i].ssd_addr = req->dma.buffer_addr + (i << HOST_DS) ;
		VPRINTF_BUFF_MGT("  %u/%u : ssd addr:0x%x ",
					  (uint32_t)i+1 ,
					  (uint32_t)req->scmd.nlbs + 1,
					  req->dma.list[i].ssd_addr ) ;
	}
	
	page_buffer_inuse[free_index-1] = true ;
	return 0;
}

int get_write_io_buffer (w_req_t *req ){
	
	u32 free_index = 0;
	u32 free_wbc_index = 0;
	
	for(u32 i = 0 ; i < N_Caches ; i++){
		if (page_buffer_inuse[i] == false){
			free_index = i+1;
			break;
		}
	}
	if (!free_index) return -1;
	
	for(u32 i = 0 ; i < N_WBCaches ; i++){
		if (wbc_inuse[i] == false){
			free_wbc_index = i+1;
			break;
		}
	}
	if (!free_wbc_index) return -1;
	
	req->dma.buffer_addr = page_buffer[free_index-1] ;
	lpn_t lpn = req->scmd.slba;
	uint16_t i =0;
	VPRINTF_BUFF_MGT(" write io page buffer req:%u " , req->index);
	for (  ; i <= req->scmd.nlbs ; i++ ){
		
		req->dma.list[i].ssd_addr = req->dma.buffer_addr + (i << HOST_DS) ;
		
		wbc.pages[i].lpn = lpn++ ;
		wbc.pages[i].ssd_addr = req->dma.list[i].ssd_addr ;
		wbc.pages[i].valid = 1;
		wbc.pages[i].nand_io_complete = false;
		
		VPRINTF_BUFF_MGT("  %u/%u : ssd addr:0x%x ", 
					  (uint32_t) i+1,
					  (uint32_t)req->scmd.nlbs+1,
					  req->dma.list[i].ssd_addr ) ;
	}

	wbc.nand_io_complete = false;
	wbc.num_program_pgs = i; 
	page_buffer_inuse[free_index-1] = true ;
	return 0;
}

unsigned char * get_page_buffer(){
	if (page_buffer_inuse) return (unsigned char *)0 ;
	
	page_buffer_inuse = true ;
	return  page_buffer ;
}


int free_read_io_buffer(r_req_t *req){	
	page_buffer_inuse = false ;
	return 0;
}

int free_write_io_buffer(w_req_t *req){
	flush_all_wbc();
	return 0;
}

int free_page_buffer(){
	page_buffer_inuse = false ;
	return 0;
} 

int flush_all_wbc(){
	sys_push_to_ftl((void*)&wbc,WBC_FLUSH_REQUEST);
	// dump stats
	//wbc_flush.raw ++;
	//setReg64(stats_dev->wbc_flush , wbc_flush );
	return 0;	
}

void free_wbc_buffer(wbc_req_t * wbc){
	
	page_buffer_inuse = false ;
	
	for (uint32_t icache = 0 ; icache < WBC_PAGES ; icache++){	
		wbc->pages[icache].valid = 0;
	}
}




