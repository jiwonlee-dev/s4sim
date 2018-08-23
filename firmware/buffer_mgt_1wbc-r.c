
 
unsigned char * page_buffer  ;
bool page_buffer_inuse = false ;

wbc_req_t wbc;

int init_buffer_mgt(){
	page_buffer_inuse = false ;
	page_buffer = get_uncacheable_ram( MDTS_PAGES * HOST_PAGE_SIZE );
	
	VPRINTF_BUFF_MGT(" Page Buffer : buffer addr:0x%x Size:%u KB" ,
				   page_buffer ,  
				   (MDTS_PAGES * HOST_PAGE_SIZE ) >> 10);
	
	wbc.index = 1;
	wbc.type = WBC_FLUSH_REQUEST ;
	
	for (uint32_t icache = 0 ; icache < WBC_PAGES ; icache++){	
		wbc.pages[icache].valid = 0;
	}
	
	return 0;
}

int get_read_io_buffer (r_req_t *req ){	
	if (page_buffer_inuse) return -1 ;

	req->dma.buffer_addr = page_buffer ;
	VPRINTF_BUFF_MGT(" read io page buffer req:%u " , req->index);
	for ( uint16_t i =0 ; i <= req->scmd.nlbs ; i++ ){
		req->dma.list[i].ssd_addr = page_buffer + (i << HOST_DS) ; 
		VPRINTF_BUFF_MGT("  %u/%u : ssd addr:0x%x ",
					  (uint32_t)i+1 ,
					  (uint32_t)req->scmd.nlbs + 1,
					  req->dma.list[i].ssd_addr ) ;
	}
	
	page_buffer_inuse = true ;
	return 0;
}

int get_write_io_buffer (w_req_t *req ){
	if (page_buffer_inuse)	return -1 ;

	req->dma.buffer_addr = page_buffer ;
	lpn_t lpn = req->scmd.slba;
	uint16_t i =0;
	VPRINTF_BUFF_MGT(" write io page buffer req:%u " , req->index);
	for (  ; i <= req->scmd.nlbs ; i++ ){
		
		req->dma.list[i].ssd_addr = page_buffer + (i << HOST_DS) ; 
		
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
	page_buffer_inuse = true ;
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
	return 0;	
}

void free_wbc_buffer(wbc_req_t * wbc){
	page_buffer_inuse = false ;
	
	for (uint32_t icache = 0 ; icache < WBC_PAGES ; icache++){	
		wbc->pages[icache].valid = 0;
	}
}




