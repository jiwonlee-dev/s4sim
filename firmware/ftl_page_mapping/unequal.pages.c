

// read modify program temporary page 
#define DIRECT_FROM_WBC 2
#define TEMP_RMP_BUFFER 3 

typedef struct program_pages_t {
	lpn_t		nand_page ;
	bool			is_unmapped ;
	ppa_t		current_ppa ;
	ppa_t		new_ppa ;
	
	uint32_t	data_src ;
	unsigned char*	temp_buffer ;
	
	lpn_t		host_page[HOST_PER_NAND_PAGES] ;
	unsigned char*	host_page_ptrs [HOST_PER_NAND_PAGES] ;
	uint32_t    mod_host_pages ;
}program_pages_t;

program_pages_t program_pages [MDTS_PAGES];
uint32_t 		program_pages_count  ;
unsigned char *		rmp_buffer[MDTS_PAGES];

// partial reading
typedef struct read_pages_t {
	lpn_t		nand_page ;
	bool			is_unmapped ;
	ppa_t		current_ppa ;
	ppa_t		new_ppa ;
	
	uint32_t	data_src ;
	unsigned char*	temp_buffer ;
	
	lpn_t		host_page[HOST_PER_NAND_PAGES] ;
	unsigned char*	host_page_ptrs [HOST_PER_NAND_PAGES] ;
	uint32_t	read_host_pages;
}read_pages_t;

read_pages_t 	read_pages [MDTS_PAGES];
uint32_t 		read_pages_count  ;
unsigned char * 		temp_read_buffer  [MDTS_PAGES];


init_ftl_module_unequal_pages(){
for (uint32_t i = 0 ; i < MDTS_PAGES ; i++){
	rmp_buffer[i] = get_uncacheable_ram(NAND_PAGE_SIZE);
}

for (uint32_t i = 0 ; i < MDTS_PAGES ; i++){
	temp_read_buffer[i] = get_uncacheable_ram(NAND_PAGE_SIZE);
}
}

void ftl_process_read_io(r_req_t * req ){
	
	lpn_t lpn ;
	uint32_t lpn_index ;
	uint32_t offset ;
	read_pages_t * nand_page_info ;
	void* rop = begin_read_op(false);
	
	/*prints*/
#ifdef VERBOSE_NAND_MODULE  
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
#endif
	
	for (uint32_t ix = 0 ; ix < read_pages_count ; ix ++){
		read_pages[ix].nand_page = 0xffffffff ;
		read_pages[ix].read_host_pages = 0 ;
		read_pages[ix].current_ppa.data = 0;
	}
	read_pages_count = 0 ;
	
	lpn = req->scmd.slba ; 
	lpn_index =0;
	
	for (; lpn_index <= req->scmd.nlbs ; lpn_index++ , lpn++){
		
		req->read_page_meta[lpn_index].lpn = lpn ; //lpn = req->pages[lpn_index].lpn ;
		lpn_t nand_page = lpn >> HOST_2_NAND_DS;
		offset = lpn % HOST_PER_NAND_PAGES ;
		
		//VPRINTF_NM ( " -- %u %u %u " ,lpn , nand_page ,offset );
		
		nand_page_info = NULL ;
		for (uint32_t ix = 0 ; ix < read_pages_count ; ix++) {
			if (nand_page == read_pages[ix].nand_page){
				nand_page_info = &(read_pages[ix]);
			}
		}
		
		if (!nand_page_info){
			
			nand_page_info = &(read_pages[read_pages_count]) ;
			
			nand_page_info->nand_page = nand_page ;
			
			for (uint32_t ih = 0 ; ih < HOST_PER_NAND_PAGES ; ih++){
				nand_page_info->host_page_ptrs[ih] = 0;
				nand_page_info->host_page[ih] = 0 ;
			}
			nand_page_info->read_host_pages = 0 ;
			
			if(IS_UNMAPPED_LPA(nand_page)){
				nand_page_info->is_unmapped = true ;
			}
			else{
				nand_page_info->is_unmapped = false ;
				nand_page_info->current_ppa.data = GET_MAPPED_PPA(nand_page) ;
				
				nand_page_info->temp_buffer = temp_read_buffer[read_pages_count] ;
				
				add_read_op_page(rop, nand_page_info->current_ppa.data, nand_page_info->temp_buffer,(void*)0);
				
				//nand_reads.raw ++;
			}
			
			read_pages_count ++ ;
		}
		
		if (nand_page_info->is_unmapped){
			req->dma.list[lpn_index].ssd_addr = ZERO_BUFFER ;
			req->read_page_meta[lpn_index].nand_io_complete = true ;
		}
		else{
			req->read_page_meta[lpn_index].nand_io_complete = false ;
			nand_page_info->host_page_ptrs[offset] = req->dma.list[lpn_index].ssd_addr ;
			nand_page_info->host_page[offset] = lpn ;
			nand_page_info->read_host_pages ++ ;
		}
		
	}
	
	sync_end_read_op(rop);
	
	/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
		simple_snprintf(p_2_ptr ,p_buffSize , "READ IO Req:%u : flash pages:%u " , req->index, read_pages_count);
		cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	}
	
	for (uint32_t in = 0 ; in < read_pages_count; in++) {
		
		nand_page_info = &(read_pages[in]);
		
		/*prints*/{ 
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
							nand_page_info->nand_page ,
							nand_page_info->current_ppa.path.chn , 
							nand_page_info->current_ppa.path.die ,
							nand_page_info->current_ppa.path.plane ,
							nand_page_info->current_ppa.path.block,
							nand_page_info->current_ppa.path.page );
			
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}
		
		if (nand_page_info->is_unmapped){
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " unmapped ] " );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
			}	
			continue ;
		}
		
		
		for (uint32_t ih=0; ih < HOST_PER_NAND_PAGES ; ih++){
			
			if (!nand_page_info->host_page_ptrs[ih] ) {
				/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
					simple_snprintf(p_2_ptr ,p_buffSize , " %u:- " , 
									ih, nand_page_info->host_page[ih] ); 
					cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
				}
				continue ;
			}
			
			
			unsigned char* src = nand_page_info->temp_buffer + (ih * HOST_PAGE_SIZE) ; 
			unsigned char* dst = nand_page_info->host_page_ptrs[ih];
			
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " %u:%u " , 
								ih, nand_page_info->host_page[ih]);
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
			}
			
			
			memcpy_sysbitw(dst,src, HOST_PAGE_SIZE / 4 );
		}
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "]"  );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
		}
	}
	
#ifdef VERBOSE_NAND_MODULE
	_sync_print_(p_ptr) ;
#endif
	
	// read is done , push to complete list
	VPRINTF_NM("done reading pages, req index:%u" , req->index);
	req->nand_io_complete = true ; 
	sys_push_to_nvme((void*)req,req->type);
}

void process_wbc(wbc_req_t *wbc){
	
	lpn_t lpn  ;
	program_pages_t * nand_page_info ;
	lpn_t nand_page ;
	uint32_t offset ;
	
	void* rop;
	void* pop ;
	
#ifdef VERBOSE_NAND_MODULE  
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
#endif
	
	
	for (uint32_t ix = 0 ; ix < program_pages_count   ; ix ++){
		program_pages [ix].nand_page = 0xffffffff ;
		program_pages [ix].mod_host_pages = 0 ;
		program_pages [ix].current_ppa.data = 0;
		program_pages [ix].new_ppa.data = 0;
	}
	program_pages_count  = 0 ;
	
	wbc->nand_io_complete = false ;
	uint32_t num_program_pgs = 0 ;
	
	for (uint32_t icache = 0 ; 
		 (icache < WBC_PAGES) && (num_program_pgs < wbc->num_program_pgs) ; 
		 icache++){	
		
		if (!wbc->pages[icache].valid){ continue ; }
		
		num_program_pgs++ ;
		lpn = wbc->pages[icache].lpn ;
		nand_page = lpn >> HOST_2_NAND_DS ;
		offset = lpn % HOST_PER_NAND_PAGES ;
		
		//VPRINTF_NM ( " -- %u %u %u " ,lpn , nand_page ,offset );
		
		wbc->pages[icache].nand_io_complete = false ;
		
		// find in nand_page in list
		nand_page_info = NULL ;
		for (uint32_t ix = 0 ; ix < program_pages_count ; ix++) {
			if (nand_page == program_pages[ix].nand_page){
				nand_page_info = &(program_pages[ix]);
			}
		}
		
		if (!nand_page_info){
			
			nand_page_info = &(program_pages[program_pages_count]) ;
			
			if(IS_UNMAPPED_LPA(nand_page)){
				nand_page_info->is_unmapped = true ;
			}
			else{
				nand_page_info->is_unmapped = false ;
				nand_page_info->current_ppa.data = GET_MAPPED_PPA(nand_page) ;
			}
			
			for (uint32_t ih = 0 ; ih < HOST_PER_NAND_PAGES ; ih++){
				nand_page_info->host_page_ptrs[ih] = 0;
			}
			
			nand_page_info->nand_page = nand_page ;
			nand_page_info->mod_host_pages = 0 ;
			nand_page_info->new_ppa.data = make_new_mapping(nand_page);
			nand_page_info->temp_buffer = rmp_buffer[program_pages_count];
			
			program_pages_count++ ;
		}
		
		nand_page_info->host_page[offset] = lpn ;
		nand_page_info->host_page_ptrs[offset] = wbc->pages[icache].ssd_addr;
		nand_page_info->mod_host_pages ++ ;
		
	}
	
	rop = begin_read_op(false);
	
	for (uint32_t ix = 0 ; ix < program_pages_count ; ix++) {
		
		nand_page_info = &(program_pages[ix]);
		
		if (nand_page_info->is_unmapped || 
			nand_page_info->mod_host_pages >= HOST_PER_NAND_PAGES){
			//  reading page is not required
			nand_page_info->data_src = DIRECT_FROM_WBC ;
		}
		else{
			// add to read list
			nand_page_info->data_src = TEMP_RMP_BUFFER ;
			
			add_read_op_page(rop, nand_page_info->current_ppa.data, nand_page_info->temp_buffer, (void*)0);
		}
	}
	
	sync_end_read_op(rop);
	
	/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
		simple_snprintf(p_2_ptr ,p_buffSize , "Flush WBC Req:%u : flash pages:%u " , 
						wbc->index, program_pages_count);
		cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	}
	
	pop = begin_program_op(false);
	
	for (uint32_t in = 0 ; in < program_pages_count ; in++) {
		
		nand_page_info = &(program_pages[in]);
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			
			if (nand_page_info->is_unmapped){	
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u unmapped -> {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
								nand_page_info->nand_page ,
								nand_page_info->new_ppa.path.chn , 
								nand_page_info->new_ppa.path.die ,
								nand_page_info->new_ppa.path.plane ,
								nand_page_info->new_ppa.path.block,
								nand_page_info->new_ppa.path.page
								);
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
			}
			else{
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u {chn:%u,%u,%u,%u,page:%u} -> {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
								nand_page_info->nand_page ,
								nand_page_info->current_ppa.path.chn , 
								nand_page_info->current_ppa.path.die ,
								nand_page_info->current_ppa.path.plane ,
								nand_page_info->current_ppa.path.block,
								nand_page_info->current_ppa.path.page ,
								nand_page_info->new_ppa.path.chn , 
								nand_page_info->new_ppa.path.die ,
								nand_page_info->new_ppa.path.plane ,
								nand_page_info->new_ppa.path.block,
								nand_page_info->new_ppa.path.page
								);
				
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
			}
			
#endif
		}
		
		for (uint32_t ih=0; ih < HOST_PER_NAND_PAGES ; ih++){
			
			if (nand_page_info->host_page_ptrs[ih] == 0){
				
				/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
					simple_snprintf(p_2_ptr ,p_buffSize , " %u:-  " , ih);
					cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
				}
				continue ;
			}
			
			unsigned char* src = nand_page_info->host_page_ptrs[ih];
			unsigned char* dst = nand_page_info->temp_buffer + (ih * HOST_PAGE_SIZE) ;
			
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " %u:%u  " , 
								ih, nand_page_info->host_page[ih] );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
			}
			
			memcpy_sysbitw(dst,src, HOST_PAGE_SIZE / 4 );
		}
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "]"  );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
		}
		
		add_program_op_page(pop, nand_page_info->new_ppa.data, nand_page_info->temp_buffer, (void*)0); 
	}
	
#ifdef VERBOSE_NAND_MODULE 
	_sync_print_(p_ptr);
#endif
	
	sync_end_program_op(pop);
	VPRINTF_NM("done flushing write back cache :%u" , wbc->index);
	// push to complete list 
	wbc->nand_io_complete =true;
	sys_push_to_nvme((void*)wbc,wbc->type);
}



u32 __sync_read_pages(lpn_t start_lpn , uint32_t num_of_pgs , unsigned char* ssd_addr){
	
	lpn_t lpn; 	uint32_t lpn_index ;
	uint32_t offset ;
	read_pages_t * nand_page_info ;
	void* rop = begin_read_op(false);
	
	/*prints*/
#ifdef VERBOSE_NAND_MODULE  
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
#endif
	
	for (uint32_t ix = 0 ; ix < read_pages_count ; ix ++){
		read_pages[ix].nand_page = 0xffffffff ;
		read_pages[ix].read_host_pages = 0 ;
		read_pages[ix].current_ppa.data = 0;
	}
	read_pages_count = 0 ;
	
	lpn = start_lpn;
	lpn_index =0;
	
	for (; lpn_index < num_of_pgs ; lpn_index++,lpn++){
		
		lpn_t nand_page = lpn >> HOST_2_NAND_DS;
		offset = lpn % HOST_PER_NAND_PAGES ;
		
		nand_page_info = NULL ;
		for (uint32_t ix = 0 ; ix < read_pages_count ; ix++) {
			if (nand_page == read_pages[ix].nand_page){
				nand_page_info = &(read_pages[ix]);
			}
		}
		
		if (!nand_page_info){
			
			nand_page_info = &(read_pages[read_pages_count]) ;
			
			nand_page_info->nand_page = nand_page ;
			
			for (uint32_t ih = 0 ; ih < HOST_PER_NAND_PAGES ; ih++){
				nand_page_info->host_page_ptrs[ih] = 0;
				nand_page_info->host_page[ih] = 0 ;
			}
			nand_page_info->read_host_pages = 0 ;
			
			if(IS_UNMAPPED_LPA(nand_page)){
				
				// pages can not be unmapped for internal processes
				return 0;
				
				//			nand_page_info->is_unmapped = true ;
			}
			else{
				nand_page_info->is_unmapped = false ;
				nand_page_info->current_ppa.data = GET_MAPPED_PPA(nand_page) ;
				
				nand_page_info->temp_buffer = temp_read_buffer[read_pages_count] ;
				
				add_read_op_page(rop, 
								 nand_page_info->current_ppa.data, 
								 nand_page_info->temp_buffer, (void*)0);
				
				//nand_reads.raw ++;
			}
			
			read_pages_count ++ ;
		}
		
		//if (nand_page_info->is_unmapped){
		//	req->pages[lpn_index].ssd_addr = ZERO_BUFFER ;
		//	req->pages[lpn_index].nand_io_complete = true ;
		//}
		//else{
		//req->pages[lpn_index].nand_io_complete = false ;
		nand_page_info->host_page_ptrs[offset] = ssd_addr + (lpn_index << HOST_DS);
		nand_page_info->host_page[offset] = lpn ;
		nand_page_info->read_host_pages ++ ;
		//}
		
	}
	
	sync_end_read_op(rop);
	
	/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
		simple_snprintf(p_2_ptr ,p_buffSize , "SYNC READ : flash pages:%u ", read_pages_count);
		cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	}
	
	for (uint32_t in = 0 ; in < read_pages_count; in++) {
		
		nand_page_info = &(read_pages[in]);
		
		/*prints*/{ 
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
							nand_page_info->nand_page ,
							nand_page_info->current_ppa.path.chn , 
							nand_page_info->current_ppa.path.die ,
							nand_page_info->current_ppa.path.plane ,
							nand_page_info->current_ppa.path.block,
							nand_page_info->current_ppa.path.page );
			
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}
		
		if (nand_page_info->is_unmapped){
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " unmapped ] " );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
			}	
			continue ;
		}
		
		
		for (uint32_t ih=0; ih < HOST_PER_NAND_PAGES ; ih++){
			
			if (!nand_page_info->host_page_ptrs[ih] ) {
				/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
					simple_snprintf(p_2_ptr ,p_buffSize , " %u:- " , 
									ih, nand_page_info->host_page[ih] ); 
					cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
				}
				continue ;
			}
			
			
			unsigned char* src = nand_page_info->temp_buffer + (ih * HOST_PAGE_SIZE) ; 
			unsigned char* dst = nand_page_info->host_page_ptrs[ih];
			
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " %u:%u " , 
								ih, nand_page_info->host_page[ih]);
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
			}
			
			
			memcpy_sysbitw(dst,src, HOST_PAGE_SIZE / 4 );
		}
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "]"  );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
		}
	}
	
#ifdef VERBOSE_NAND_MODULE
	_sync_print_(p_ptr) ;
#endif
	
	VPRINTF_NM("done sync read pages ");
	return read_pages_count;
}


u32 __sync_program_pages(lpn_t start_lpn, u32 num_of_lpn, u8* ssd_addr){
	
	lpn_t lpn  ;
	program_pages_t * nand_page_info ;
	lpn_t nand_page ;
	uint32_t offset ;
	
	void* rop;
	void* pop ;
	
#ifdef VERBOSE_NAND_MODULE  
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
#endif
	
	
	for (uint32_t ix = 0 ; ix < program_pages_count   ; ix ++){
		program_pages [ix].nand_page = 0xffffffff ;
		program_pages [ix].mod_host_pages = 0 ;
		program_pages [ix].current_ppa.data = 0;
		program_pages [ix].new_ppa.data = 0;
	}
	program_pages_count  = 0 ;
	
	//wbc->nand_io_complete = false ;
	uint32_t num_program_pgs = 0 ;
	
	lpn = start_lpn;
	for (u32 lpn_index = 0; 	lpn_index < num_of_lpn; lpn_index++, lpn++){	
		
		num_program_pgs++ ;
		//lpn = wbc->pages[icache].lpn ;
		nand_page = lpn >> HOST_2_NAND_DS ;
		offset = lpn % HOST_PER_NAND_PAGES ;
		
		//VPRINTF_NM ( " -- %u %u %u " ,lpn , nand_page ,offset );
		
		//wbc->pages[icache].nand_io_complete = false ;
		
		// find in nand_page in list
		nand_page_info = NULL ;
		for (uint32_t ix = 0 ; ix < program_pages_count ; ix++) {
			if (nand_page == program_pages[ix].nand_page){
				nand_page_info = &(program_pages[ix]);
			}
		}
		
		if (!nand_page_info){
			
			nand_page_info = &(program_pages[program_pages_count]) ;
			
			if(IS_UNMAPPED_LPA(nand_page)){
				nand_page_info->is_unmapped = true ;
			}
			else{
				nand_page_info->is_unmapped = false ;
				nand_page_info->current_ppa.data = GET_MAPPED_PPA(nand_page) ;
			}
			
			for (uint32_t ih = 0 ; ih < HOST_PER_NAND_PAGES ; ih++){
				nand_page_info->host_page_ptrs[ih] = 0;
			}
			
			nand_page_info->nand_page = nand_page ;
			nand_page_info->mod_host_pages = 0 ;
			nand_page_info->new_ppa.data = make_new_mapping(nand_page);
			nand_page_info->temp_buffer = rmp_buffer[program_pages_count];
			
			program_pages_count++ ;
		}
		
		nand_page_info->host_page[offset] = lpn ;
		nand_page_info->host_page_ptrs[offset] = ssd_addr + (lpn_index << HOST_DS); 
		nand_page_info->mod_host_pages ++ ;
		
	}
	
	rop = begin_read_op(false);
	
	for (uint32_t ix = 0 ; ix < program_pages_count ; ix++) {
		
		nand_page_info = &(program_pages[ix]);
		
		if (nand_page_info->is_unmapped || 
			nand_page_info->mod_host_pages >= HOST_PER_NAND_PAGES){
			//  reading page is not required
			nand_page_info->data_src = DIRECT_FROM_WBC ;
		}
		else{
			// add to read list
			nand_page_info->data_src = TEMP_RMP_BUFFER ;
			
			add_read_op_page(rop, nand_page_info->current_ppa.data, nand_page_info->temp_buffer, (void*)0);
		}
	}
	
	sync_end_read_op(rop);
	
	/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
		simple_snprintf(p_2_ptr ,p_buffSize , "SYNC PROGRAM : flash pages:%u " , 
						program_pages_count);
		cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	}
	
	pop = begin_program_op(false);
	
	for (uint32_t in = 0 ; in < program_pages_count ; in++) {
		
		nand_page_info = &(program_pages[in]);
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			
			if (nand_page_info->is_unmapped){	
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u unmapped -> {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
								nand_page_info->nand_page ,
								nand_page_info->new_ppa.path.chn , 
								nand_page_info->new_ppa.path.die ,
								nand_page_info->new_ppa.path.plane ,
								nand_page_info->new_ppa.path.block,
								nand_page_info->new_ppa.path.page
								);
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
			}
			else{
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u {chn:%u,%u,%u,%u,page:%u} -> {chn:%u,%u,%u,%u,page:%u} lpns:[" , 
								nand_page_info->nand_page ,
								nand_page_info->current_ppa.path.chn , 
								nand_page_info->current_ppa.path.die ,
								nand_page_info->current_ppa.path.plane ,
								nand_page_info->current_ppa.path.block,
								nand_page_info->current_ppa.path.page ,
								nand_page_info->new_ppa.path.chn , 
								nand_page_info->new_ppa.path.die ,
								nand_page_info->new_ppa.path.plane ,
								nand_page_info->new_ppa.path.block,
								nand_page_info->new_ppa.path.page
								);
				
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
			}
			
#endif
		}
		
		for (uint32_t ih=0; ih < HOST_PER_NAND_PAGES ; ih++){
			
			if (nand_page_info->host_page_ptrs[ih] == 0){
				
				/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
					simple_snprintf(p_2_ptr ,p_buffSize , " %u:-  " , ih);
					cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
				}
				continue ;
			}
			
			unsigned char* src = nand_page_info->host_page_ptrs[ih];
			unsigned char* dst = nand_page_info->temp_buffer + (ih * HOST_PAGE_SIZE) ;
			
			/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
				simple_snprintf(p_2_ptr ,p_buffSize , " %u:%u  " , 
								ih, nand_page_info->host_page[ih] );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif		
			}
			
			memcpy_sysbitw(dst,src, HOST_PAGE_SIZE / 4 );
		}
		
		/*prints*/{
#ifdef VERBOSE_NAND_MODULE 
			simple_snprintf(p_2_ptr ,p_buffSize , "]"  );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
		}
		
		add_program_op_page(pop, nand_page_info->new_ppa.data, nand_page_info->temp_buffer, (void*)0); 
	}
	
#ifdef VERBOSE_NAND_MODULE 
	_sync_print_(p_ptr);
#endif
	
	sync_end_program_op(pop);
	VPRINTF_NM("done sync program pages");
	return program_pages_count;
}
