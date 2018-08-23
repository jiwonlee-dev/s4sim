
// private print buffers for large debug text
#ifdef VERBOSE_FTL_DETAILED
#define p_buffSize 0x2000
char p_buff	 [0x10000];
char p_buff_2	 [0x2000];
#endif


void ftl_process_read_io(r_req_t * req ){
	lpn_t lpn ;
	uint32_t lpn_index ;
	void* rop;
	ppa_t current_ppa ;

	req->req_stage = REQ_STAGE_FTL ;
	
#ifdef VERBOSE_FTL_DETAILED
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
	simple_snprintf(p_2_ptr ,p_buffSize , "READ IO Req:%u : flash pages:%u " , req->index, req->scmd.nlbs+1);
	cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	
	rop = begin_read_op(false);
	lpn = req->scmd.slba ; 
	lpn_index =0;
	
	for (; lpn_index <= req->scmd.nlbs ; lpn_index++ , lpn++){
		req->read_page_meta[lpn_index].lpn = lpn ; 

		if(IS_UNMAPPED_LPA(lpn)){
			
			// copy zero buffer
			u8* dst = req->dma.buffer_addr + (lpn_index << HOST_DS) ; 
			memset32((u32*) dst , 0 , HOST_PAGE_SIZE / 4);
			
			/*prints*/{
#ifdef VERBOSE_FTL_DETAILED
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u { unmapped } " , lpn );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
			}	
		}
		else{
			current_ppa.data = GET_MAPPED_PPA(lpn) ;
			u8* ssd_addr = req->dma.buffer_addr + ( lpn_index << HOST_DS ) ;
			add_read_op_page(rop, current_ppa.data, ssd_addr ,(void*)0);
			//nand_reads.raw ++;
			
			/*prints*/{ 
#ifdef VERBOSE_FTL_DETAILED
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u { chn:%u,%u,%u,%u,page:%u } ->> 0x%x" , 
								lpn,
								current_ppa.path.chn , 
								current_ppa.path.die ,
								current_ppa.path.plane ,
								current_ppa.path.block,
								current_ppa.path.page,
								ssd_addr );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
			}
		}
			
	}
		
#ifdef VERBOSE_FTL_DETAILED
	_sync_print_(p_ptr) ;
#endif

	req->req_stage = REQ_STAGE_READING_FLASH ;
	sync_end_read_op(rop);
	// read is done , push to complete list
	VPRINTF_FTL("done reading pages, req index:%u" , req->index);
	req->nand_io_complete = true ; 
	sys_push_to_nvme((void*)req,req->type);
}

void process_wbc(wbc_req_t *wbc){
	ppa_t new_ppa;
	lpn_t lpn  ;
	void* pop ;
	
#ifdef VERBOSE_FTL_DETAILED
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
	simple_snprintf(p_2_ptr ,p_buffSize , "Flush WBC Req:%u : flash pages:%u " ,wbc->index, wbc->num_program_pgs);
		cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	
	pop = begin_program_op(false);
	wbc->nand_io_complete = false ;
	uint32_t num_program_pgs = 0 ;
	
	for (uint32_t icache = 0 ; 
		 (icache < WBC_PAGES) && (num_program_pgs < wbc->num_program_pgs) ; 
		 icache++)
	{	
		
		if (!wbc->pages[icache].valid){ continue ; }
		lpn = wbc->pages[icache].lpn ;
		
		if(IS_UNMAPPED_LPA(lpn)){
			
		new_ppa.data = make_new_mapping(lpn);
	
#ifdef VERBOSE_FTL_DETAILED
			simple_snprintf(p_2_ptr ,p_buffSize , 
							"\n\tnand page:%u { unmapped } -> { chn:%u,%u,%u,%u,page:%u } <<- 0x%x" , 
							lpn ,
							new_ppa.path.chn , 
							new_ppa.path.die ,
							new_ppa.path.plane ,
							new_ppa.path.block,
							new_ppa.path.page,
							wbc->pages[icache].ssd_addr );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}else{

#ifdef VERBOSE_FTL_DETAILED	
			ppa_t current_ppa ;		
			current_ppa.data = GET_MAPPED_PPA(lpn);
#endif

			new_ppa.data = make_new_mapping(lpn);
			
#ifdef VERBOSE_FTL_DETAILED
			simple_snprintf(p_2_ptr ,p_buffSize,
							"\n\tnand page:%u { chn:%u,%u,%u,%u,page:%u } -> { chn:%u,%u,%u,%u,page:%u } <<- 0x%x" , 
							lpn ,
							current_ppa.path.chn , 
							current_ppa.path.die ,
							current_ppa.path.plane ,
							current_ppa.path.block,
							current_ppa.path.page ,
							new_ppa.path.chn , 
							new_ppa.path.die ,
							new_ppa.path.plane ,
							new_ppa.path.block,
							new_ppa.path.page,
							wbc->pages[icache].ssd_addr);
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}
	
		wbc->pages[icache].nand_io_complete = false ;
		add_program_op_page(pop, new_ppa.data, wbc->pages[icache].ssd_addr, (void*)0); 
	}
	
#ifdef VERBOSE_FTL_DETAILED
	_sync_print_(p_ptr);
#endif
	
	sync_end_program_op(pop);
	VPRINTF_FTL("done flushing write back cache :%u" , wbc->index);
	// push to complete list 
	wbc->nand_io_complete =true;
	sys_push_to_nvme((void*)wbc,wbc->type);
}

u32 __sync_read_pages(lpn_t start_lpn , uint32_t num_of_lpg , u8* lpg_buffer ){
	
	lpn_t lpn ;
	uint32_t lpn_index ;
	void* rop;
	ppa_t current_ppa ;
	
#ifdef VERBOSE_FTL_DETAILED
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
	simple_snprintf(p_2_ptr ,p_buffSize , "SYNC READ : flash pages:%u " , num_of_lpg );
	cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	
	rop = begin_read_op(false);
	lpn = start_lpn;
	lpn_index =0;
	
	for (; lpn_index < num_of_lpg ; lpn_index++ , lpn++){
	
		
		if(IS_UNMAPPED_LPA(lpn)){
			
			// copy zero buffer
			u8* dst = lpg_buffer + ( lpn_index << HOST_DS );
			memset32((u32*) dst , 0 , HOST_PAGE_SIZE / 4);
			
			/*prints*/{
#ifdef VERBOSE_FTL_DETAILED
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u { unmapped } " , lpn );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif	
			}	
		}
		else{
			current_ppa.data = GET_MAPPED_PPA(lpn) ;
			u8* ssd_addr = lpg_buffer + ( lpn_index << HOST_DS ) ;
			add_read_op_page(rop, current_ppa.data, ssd_addr ,(void*)0);
			//nand_reads.raw ++;
			
			/*prints*/{ 
#ifdef VERBOSE_FTL_DETAILED
				simple_snprintf(p_2_ptr ,p_buffSize , "\n\tnand page:%u { chn:%u,%u,%u,%u,page:%u } ->> 0x%x" , 
								lpn,
								current_ppa.path.chn , 
								current_ppa.path.die ,
								current_ppa.path.plane ,
								current_ppa.path.block,
								current_ppa.path.page,
								ssd_addr );
				cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
			}
		}
		
	}
	
#ifdef VERBOSE_FTL_DETAILED
	_sync_print_(p_ptr) ;
#endif
	
	sync_end_read_op(rop);
	VPRINTF_FTL("done sync read pages");
	return num_of_lpg;
}


u32 __sync_program_pages(lpn_t start_lpn, u32 num_of_lpg, u8* lpg_buffer ){
	ppa_t new_ppa;			
	lpn_t lpn ;
	u32 lpn_index ;
	void* pop;
	
#ifdef VERBOSE_FTL_DETAILED
	char * p_ptr = &(p_buff[0]) ;
	char * p_2_ptr = &(p_buff_2[0]);
	simple_snprintf(p_ptr,p_buffSize,"");
	char * cat_p_ptr = p_ptr ;
	simple_snprintf(p_2_ptr ,p_buffSize , "SYNC PROGRAM : flash pages:%u " , num_of_lpg );
	cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
	
	pop = begin_program_op(false);
	lpn = start_lpn;
	lpn_index =0;
	
	for (; lpn_index < num_of_lpg ; lpn_index++ , lpn++){
		
		u8* ssd_addr = lpg_buffer + (lpn_index << HOST_DS);
		
		if(IS_UNMAPPED_LPA(lpn)){
			new_ppa.data = make_new_mapping(lpn);
			
#ifdef VERBOSE_FTL_DETAILED
			simple_snprintf(p_2_ptr ,p_buffSize , 
							"\n\tnand page:%u { unmapped } -> { chn:%u,%u,%u,%u,page:%u } <<- 0x%x" , 
							lpn ,
							new_ppa.path.chn , 
							new_ppa.path.die ,
							new_ppa.path.plane ,
							new_ppa.path.block,
							new_ppa.path.page,
							ssd_addr );
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}else{
			
#ifdef VERBOSE_FTL_DETAILED		
			ppa_t current_ppa ;	
			current_ppa.data = GET_MAPPED_PPA(lpn);
#endif

			new_ppa.data = make_new_mapping(lpn);
		
#ifdef VERBOSE_FTL_DETAILED	
			simple_snprintf(p_2_ptr ,p_buffSize,
							"\n\tnand page:%u { chn:%u,%u,%u,%u,page:%u } -> { chn:%u,%u,%u,%u,page:%u } <<- 0x%x" , 
							lpn ,
							current_ppa.path.chn , 
							current_ppa.path.die ,
							current_ppa.path.plane ,
							current_ppa.path.block,
							current_ppa.path.page ,
							new_ppa.path.chn , 
							new_ppa.path.die ,
							new_ppa.path.plane ,
							new_ppa.path.block,
							new_ppa.path.page,
							ssd_addr);
			cat_p_ptr = simple_strcat(cat_p_ptr,(const char *)p_2_ptr);
#endif
		}
		
		add_program_op_page(pop, new_ppa.data, ssd_addr, (void*)0); 
	}
	
#ifdef VERBOSE_FTL_DETAILED
	_sync_print_(p_ptr) ;
#endif
	
	sync_end_program_op(pop);
	VPRINTF_FTL("done sync program pages");
	return num_of_lpg;
}

