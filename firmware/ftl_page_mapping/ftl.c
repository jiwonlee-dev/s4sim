 
 
#include "ftl.h"

// defined in linker script
extern u8 _mapping_table_start_;
extern u8 _mapping_table_end_;

ftl_config_t ftl_config;
ppa_t last_searched_channel ;

__attribute__ ((section("mapping_tables")))
block_meta_t blk_table[NUM_CHANNELS] [DIES_PER_CHANNEL] [PLANES_PER_DIE] [BLOCKS_PER_PLANE] ;

__attribute__ ((section("mapping_tables")))
ppa_t last_page_per_plane[NUM_CHANNELS][DIES_PER_CHANNEL][PLANES_PER_DIE];

  
u8*		ZERO_BUFFER;
u8* 	private_pg_buff;
u32 	load_mp_buffer_size = (1 << 16) ;
u32 	load_mp_buffer_pages = (1 << 16) / NAND_PAGE_SIZE ; 
u8* 	load_mp_buffer ;





int init_ftl_module(){
	
#if HOST_2_NAND_DS != 0
	extern void init_ftl_module_unequal_pages();
	init_ftl_module_unequal_pages();
#endif

	private_pg_buff = get_uncacheable_ram(NAND_PAGE_SIZE*4);
	load_mp_buffer = get_uncacheable_ram ( load_mp_buffer_size ) ;
	ZERO_BUFFER = private_pg_buff;
	
	// set the main callback function for handling all request pushed to FTL module
	process_req_cbk_ftn_id = sys_add_thread_function( FTL_MODULE_THREAD , process_req_cbk);

	int ftn_id = sys_add_thread_function( FTL_MODULE_THREAD , initialize_ftl);
	sys_add_callback_ftn(ftn_id, 0 );

	return 0;
}

void initialize_ftl(uint32_t arg1){
	
	VPRINTF_STARTUP("############      INIT FTL       ############");
	
	// read first page in flash  
	unsigned char* buffer = ZERO_BUFFER; 
	ppa_t first_page;
	first_page.data = 0;
	void* rop = begin_read_op(false);
	add_read_op_page(rop, first_page.data,buffer,(void*)0);
	sync_end_read_op(rop);
	
	// copy data to cached area
	memcpy8((uint8_t *)&ftl_config , (uint8_t*) buffer , sizeof(ftl_config_t) );
	// reset buffer to zero
	memset32((uint32_t*)ZERO_BUFFER , 0 , NAND_PAGE_SIZE / 4 );
	
	if(ftl_config.nand_ds < 9 ){
		
		VPRINTF_STARTUP ( "  *** New flash media ***");
		
		// TODO revise or add to table generation
		for (uint32_t ichn = 0 ; ichn < NUM_CHANNELS ; ichn++){
			for( uint32_t idie = 0; idie < DIES_PER_CHANNEL ; idie++){
				for ( uint32_t iplane =0 ; iplane < PLANES_PER_DIE ; iplane++ ){
					last_page_per_plane[ichn][idie][iplane].path.chn = ichn ;
					last_page_per_plane[ichn][idie][iplane].path.die = idie ;
					last_page_per_plane[ichn][idie][iplane].path.plane = iplane;
					last_page_per_plane[ichn][idie][iplane].path.block = 0;
					last_page_per_plane[ichn][idie][iplane].path.page = 0;
				}
			}
		}
		
		// set config values 
		ftl_config.nand_ds = NAND_DS ;
		ftl_config.host_ds = HOST_DS ;
		ftl_config.pages_per_block = PAGES_PER_BLOCK ;
		ftl_config.total_pages = TOTAL_NUM_OF_PAGES ;
		
		ftl_config.rpp_ovp = (TOTAL_NUM_OF_PAGES / 100) * OVP ;
		
		u8* start_address = &_mapping_table_start_ ;
		u8* end_address	= &_mapping_table_end_;
		ftl_config.rpp_map_tables = (((end_address - start_address ) / NAND_PAGE_SIZE ) + 1);
		
		ftl_config.total_rpp = ftl_config.rpp_ovp +  ftl_config.rpp_map_tables ;
		
		ftl_config.total_logical_pages = (ftl_config.total_pages - ftl_config.total_rpp) << HOST_2_NAND_DS ;
		
		// set first page as reserved in mapping table
		SET_RESERVED_PPA(first_page);
		// update block info table
		INC_VALID_PAGES(first_page);
		ftl_config.total_invalid_pages = 0;
		ftl_config.total_valid_pages   = 1;
		ftl_config.total_free_pages    = ftl_config.total_pages - 1;
		
		first_page.path.page++;
		ftl_config.last_selected_ppa.data = first_page.data ;
		last_page_per_plane[first_page.path.chn][first_page.path.die][first_page.path.plane].data = first_page.data ;
		
	}
	else {
		
		VPRINTF_STARTUP ( "  *** Load flash tables ***");
		restore_mapping_tables();
	}
	
	ssd_config.ncap = ssd_config.nuse = ssd_config.nsze = ftl_config.total_logical_pages;
	ssd_config.host_ds = ftl_config.host_ds ; 													    

	VPRINTF_STARTUP ("\tSSD Config: \
					\n\thost page size:%u, nand page size:%u nand pages:%u \
					\n\treserved: ovp:%u, translation tables:%u  \
					\n\tnamespace: ds:%u, nsze:%u, capacity:%uMB " , 
					(unsigned int )(1 << ftl_config.host_ds) ,
					(unsigned int )(1 << ftl_config.nand_ds) ,
					(unsigned int )ftl_config.total_pages ,
					(unsigned int )ftl_config.rpp_ovp ,
					(unsigned int )ftl_config.rpp_map_tables,
					(unsigned int )ssd_config.host_ds ,
					(unsigned int )ssd_config.nsze ,
					(unsigned int )(ssd_config.nsze >> (20 - HOST_DS))
					);
	
	//stats_dev->nand_module[module_index].invalid_pages = invalid_pages[module_index] ;
	//stats_dev->nand_module[module_index].free_pages  = free_pages[module_index] ;
	//stats_dev->nand_module[module_index].valid_pages = valid_pages[module_index] ;


	last_searched_channel.data = ftl_config.last_selected_ppa.data ;
}



int find_free_page( ppa_t * new_ppa ){
	
	uint32_t nextChn = last_searched_channel.path.chn + 1 ; // ftl_config.last_selected_ppa.path.chn + 1 ;
	uint32_t next_die = last_searched_channel.path.die ;//  ftl_config.last_selected_ppa.path.die;
	uint32_t next_plane = last_searched_channel.path.plane ; // ftl_config.last_selected_ppa.path.plane ;
	
	if (nextChn >= NUM_CHANNELS ){
		nextChn = 0 ;
		
		next_die++ ;
		if (next_die >= DIES_PER_CHANNEL ){
			next_die = 0;
			
			next_plane ++;
			if (next_plane >= PLANES_PER_DIE  ){
				next_plane = 0 ;
			}
		} 
	}
	
	ppa_t i_ppa ;
	i_ppa.data = last_page_per_plane[nextChn][next_die][next_plane].data ;
	
	//uint32_t min_erase_count;
	//uint16_t min_block_index;

	
	for ( uint32_t i = 0 ; i < BLOCKS_PER_PLANE ; i ++){
		
		for (; i_ppa.path.page < PAGES_PER_BLOCK ; i_ppa.path.page ++){
			
			if ( IS_FREE_PPA(i_ppa)) { 
				
				new_ppa->data = i_ppa.data ; 
				
				i_ppa.path.page++ ;
				
				ftl_config.last_selected_ppa.data = i_ppa.data ;
				last_searched_channel.data = i_ppa.data ;

				last_page_per_plane[i_ppa.path.chn][i_ppa.path.die][i_ppa.path.plane].data = i_ppa.data;
				
				return 1 ;
			}
		}
		

		// simply moving to next block for now
		i_ppa.path.block ++ ;
		i_ppa.path.page = 0;
		if (i_ppa.path.block > BLOCKS_PER_PLANE ){
			i_ppa.path.block = 0 ;
		}




		/*
		uint16_t current_blk = i_ppa.path.block;
		// no free page in current block , select next block in current plane with least erase count
		VPRINTF_NM_WL( "\t selecting another block in plane: [chn:%u,die:%u,plane:%u] current block:%u , blk free pgs:%u " , 
					  (uint32_t)i_ppa.path.chn , 
					  (uint32_t)i_ppa.path.die , 
					  (uint32_t)i_ppa.path.plane ,
					  (uint32_t)i_ppa.path.block ,
					  GET_FREE_PAGES(i_ppa)
					  );
		 		
		min_erase_count = 0xffffffff;
		
		for (i_ppa.path.block = 0; i_ppa.path.block < BLOCKS_PER_PLANE; i_ppa.path.block++){
			
			if ( i_ppa.path.block == current_blk ){
				// skip the current block 
			}
			else if (! ( GET_FREE_PAGES(i_ppa))){
				// skip blocks with no free pages
			}
			else if (GET_BLOCK_ERASE_COUNT(i_ppa) < min_erase_count){
				min_erase_count = GET_BLOCK_ERASE_COUNT(i_ppa);
				min_block_index = i_ppa.path.block;
			}
		}
		
		i_ppa.path.block = min_block_index ; 
		i_ppa.path.page = 0 ;
		
		VPRINTF_NM_WL( "\t next block [chn:%u,die:%u,plane:%u, block:%u , blk free pgs:%u ]" , 
					  (uint32_t)i_ppa.path.chn , 
					  (uint32_t)i_ppa.path.die , 
					  (uint32_t)i_ppa.path.plane ,
					  (uint32_t)i_ppa.path.block ,
					  GET_FREE_PAGES(i_ppa)
					  );
		*/




	}
	// no free page in plane 

	last_searched_channel.data = i_ppa.data ;

	return 0 ;
}


u32 loop_limit = PAGES_PER_PLANE ;
u32 foregoround_trigger = TOTAL_NUM_OF_PAGES >> 3 ;

void get_free_ppa (ppa_t * new_ppa ){
	
	uint32_t loop = 0;

	//u32 channel = ftl_config.last_selected_ppa.path.chn + 1 ;

	while (! find_free_page(new_ppa)) {
		
		if(++loop > loop_limit){
			// do foregoround garbage collection
			//do_FGCG(module_index,new_ppa);
			//get_free_page[module_index].raw ++ ; // stats

			VPRINTF_CONFIG ( " **** Get free page loop limit reached **** \
				 	\n\tSet Limit:%u, \n\tTotal Pages:%u, \n\tTotal Valid Pages:%u,\
			 		\n\tTotal Invalid Pages:%u, \n\tTotal Free Pages:%u " ,
					loop_limit ,
					ftl_config.total_pages,
					ftl_config.total_valid_pages,
					ftl_config.total_invalid_pages ,
					ftl_config.total_free_pages  );

			// continue loop
			loop = 0 ;
			//loop_limit = PAGES_PER_PLANE >> 1 ; // TODO Garbage collector will reset this value  
			
			if (ftl_config.total_free_pages <= foregoround_trigger ){
				VPRINTF_CONFIG (" ** Calling Forground Garbage Collector (Trigger:%u) **" , foregoround_trigger ) ;
				//TODO
			}

			// change next channel
			//channel ++ ;
			//if ( channel >= NUM_CHANNELS ) {
			//	channel = 0 ;
				//nextChn = 0 
			//if ()

		}
	}

	//get_free_page[module_index].raw ++ ; //stats
	
#ifdef VERBOSE_WEAR_LEVELING
	ppa_t ppa ;
	ppa.data = new_ppa->data ;
	
	VPRINTF_NM_WL( "\t get_free_page : [chn:%u,die:%u,plane:%u,blk:%u,p:%u,erase_count:%u]" , 
			   new_ppa->path.chn , 
			   new_ppa->path.die , new_ppa->path.plane ,
			   new_ppa->path.block, new_ppa->path.page,
			   GET_BLOCK_ERASE_COUNT(ppa));
		
#endif
	
}


ppa_data_t make_new_mapping(  lpn_t lpn){
	
	ppa_t current_mapped_ppa;
	ppa_t new_ppa ;
	
	// get a new page
	get_free_ppa( &new_ppa) ;
 	
	if (IS_UNMAPPED_LPA(lpn) ){
		 
		DPRINTF_LESS ( "\n  lba not mapped lba:%u , assign free page" , lpn  );
		
		// map
		MAP_LPA_AND_PPA(lpn , new_ppa );
		
		// update block info
		INC_VALID_PAGES(new_ppa);
		
		ftl_config.total_valid_pages ++;
		ftl_config.total_free_pages -- ;
	}
	else {
		
		// get current mapping
		current_mapped_ppa.data = GET_MAPPED_PPA(lpn);
		
		// mark page as stalled 
		SET_INVALID_PPA(current_mapped_ppa);
		
		// update block info table
		INC_INVALID_PAGES(current_mapped_ppa);  
		DEC_VALID_PAGES(current_mapped_ppa);
		
		// new mapping
		MAP_LPA_AND_PPA(lpn , new_ppa );
		
		// update block info table
		INC_VALID_PAGES(new_ppa);
		
		ftl_config.total_invalid_pages ++ ;
		ftl_config.total_free_pages -- ;
	} 
	
	return new_ppa.data ;
}
 
#if HOST_2_NAND_DS == 0
#include "equal.pages.c"
#else
#include "unequal.pages.c"
#endif


void process_isp_read_pages(void* __req ){
	isp_nand_op_t* req = (isp_nand_op_t*)__req;
	
	VPRINTF (" {%s}: index:%u start_lpn:%u num_of_lpn:%u, ssd_addr:0x%x " , __FUNCTION__, 
					req->index , req->start_lpn, req->num_of_lpn, req->ssd_addr );
	
	if (req->cbk_ftn_id == SYNC_CALL){
		req->status = __sync_read_pages(req->start_lpn , req->num_of_lpn , req->ssd_addr) ;
		req->req_stage = ISP_NAND_OP_CMPLT ;
	}
}

void process_isp_program_pages(void* __req){
	isp_nand_op_t* req = (isp_nand_op_t*)__req;
	
	VPRINTF (" {%s}: index:%u start_lpn:%u num_of_lpn:%u, ssd_addr:0x%x " , __FUNCTION__, 
					req->index , req->start_lpn, req->num_of_lpn, req->ssd_addr );
	
	if (req->cbk_ftn_id == SYNC_CALL){
		req->status = __sync_program_pages(req->start_lpn , req->num_of_lpn , req->ssd_addr) ;
		req->req_stage = ISP_NAND_OP_CMPLT ;
	} 
}

void process_req_cbk(void * req , uint32_t req_type ){	
	//VPRINTF_CONFIG (" request: req_index:%u type:%u " , ((r_req_t*)req)->index , req_type );
	switch (req_type) {
		case READ_IO_REQUEST:
			ftl_process_read_io((r_req_t*)req );
			break;
		case WBC_FLUSH_REQUEST :
			process_wbc((wbc_req_t*)req);
			break ;
		case ISP_READ_PAGES_REQUEST:
			process_isp_read_pages(req);
			break ;
		case ISP_PROGRAM_PAGES_REQUEST:
			process_isp_program_pages(req);
			break;
		//case TRIM_IO_REQUEST :
			//process_trim((trim_req_t*) req );
		//	break;
		default:
			VPRINTF_ERROR (" ** %s: unknown request: req_index:%u type:%u " , __FUNCTION__, ((r_req_t*)req)->index, req_type );
			break;
	}
}


void store_mapping_tables(){
	
	VPRINTF_STARTUP  ( "FLUSH MAPPING TABLE " );
	u8* buffer_addr;
	u32 bytes_remaining;
	ppa_t* mapping_tables;
	ppa_t free_page, first_page;
	void *pop, *pop_2;
	u8* current_map_tables_addr;
	u32 current_pg_index;
	u32 current_data_len;
	
	buffer_addr = private_pg_buff;
	bytes_remaining = ((&_mapping_table_end_) - (&_mapping_table_start_)) ;
	
	VPRINTF_FTL  ("map tables: start:0x%x, end:0x%x , total bytes:%d, pages required:%u " ,
					  (&_mapping_table_start_) , (&_mapping_table_end_) ,
					  bytes_remaining, ftl_config.rpp_map_tables );

	// get free pages and mark them as INVALID in mapping table , before storing the table
	mapping_tables = & (ftl_config.mapping_tables[0]);
	
	for (u32 i = 0 ; i < ftl_config.rpp_map_tables ; i++ ){
		get_free_ppa(&free_page) ;
		mapping_tables[i].data = free_page.data;
		
		SET_INVALID_PPA(free_page);
		
		INC_INVALID_PAGES(free_page);
		DEC_VALID_PAGES(free_page);
	}
	
	VPRINTF_FTL ("start programming map data");
	current_map_tables_addr = (&_mapping_table_start_);
	current_pg_index = 0;
	
	while(bytes_remaining > 0){
		
		if (bytes_remaining < NAND_PAGE_SIZE ){
			// set buffer to zeros since not all the nand page will be needed
			memset8((buffer_addr + bytes_remaining), 0 , (NAND_PAGE_SIZE - bytes_remaining));
			current_data_len = bytes_remaining ;
		}
		else{
			current_data_len = NAND_PAGE_SIZE ;
		}
		bytes_remaining -= current_data_len;
		
		VPRINTF_FTL ( "\t %u ppa:[chn:%u,die:%u,plane:%u,blk:%u,p:%u] , tables data addr:0x%x, size:%u, bytes remaining:%u" ,
						 current_pg_index,
						 mapping_tables[current_pg_index].path.chn ,
						 mapping_tables[current_pg_index].path.die ,
						 mapping_tables[current_pg_index].path.plane ,
						 mapping_tables[current_pg_index].path.block,
						 mapping_tables[current_pg_index].path.page,
						 current_map_tables_addr, current_data_len, bytes_remaining) ;
		
		// copy data to uncached buffer
		memcpy8 (buffer_addr, current_map_tables_addr, current_data_len);
		// add page program operation
		pop = begin_program_op(false);
		add_program_op_page(pop, mapping_tables[current_pg_index].data, buffer_addr, (void*)0);
		// do page program
		sync_end_program_op(pop);
		
		current_map_tables_addr += current_data_len;
		current_pg_index ++;
	}
	
	// write first n(3) page in flash
	// copy data to uncached buffer
	memcpy8((uint8_t *)buffer_addr, (uint8_t *)&ftl_config ,  sizeof(ftl_config_t) );
	
	pop_2 = begin_program_op(false);
	first_page.data = 0;
	add_program_op_page(pop_2, first_page.data, buffer_addr, (void*)0);
	first_page.path.page++;
	add_program_op_page(pop_2, first_page.data, buffer_addr + NAND_PAGE_SIZE, (void*)0);
	first_page.path.page++;
	add_program_op_page(pop_2, first_page.data, buffer_addr + NAND_PAGE_SIZE + NAND_PAGE_SIZE, (void*)0);
	// do page program
	sync_end_program_op(pop_2);

	VPRINTF_STARTUP ("done");
}


void restore_mapping_tables(){
	
	VPRINTF_STARTUP  ( "LOAD MAPPING TABLE " );
	u8* buffer_addr;
	u32 bytes_remaining;
	ppa_t* mapping_tables;
	//ppa_t free_page;
	ppa_t first_page;
	void *rop, *rop_2;
	u8* current_map_tables_addr;
	u32 current_pg_index;
	u32 current_data_len;
	u32 current_page_count ;

	
	buffer_addr = load_mp_buffer ;// private_pg_buff;
	bytes_remaining = ((&_mapping_table_end_) - (&_mapping_table_start_)) ;
	
	// read first n(3) page in flash
	rop_2 = begin_read_op(false);
	first_page.data = 0;
	add_read_op_page(rop_2, first_page.data, buffer_addr, (void*)0);
	first_page.path.page++;
	add_read_op_page(rop_2, first_page.data, buffer_addr + NAND_PAGE_SIZE, (void*)0);
	first_page.path.page++;
	add_read_op_page(rop_2, first_page.data, buffer_addr + NAND_PAGE_SIZE + NAND_PAGE_SIZE, (void*)0);
	// do page program
	sync_end_read_op(rop_2);
	// copy data to uncached buffer
	memcpy8((uint8_t *)&ftl_config , (uint8_t *)buffer_addr,  sizeof(ftl_config_t) );
	
	
	VPRINTF_FTL  ("map tables: start:0x%x, end:0x%x , total bytes:%d, pages required:%u " ,
					  (&_mapping_table_start_) , (&_mapping_table_end_) ,
					  bytes_remaining, ftl_config.rpp_map_tables );
	
	mapping_tables = & (ftl_config.mapping_tables[0]);
	
	VPRINTF_FTL ("start reading map data");
	current_map_tables_addr = (&_mapping_table_start_);
	current_pg_index = 0;
	current_page_count = 0;	

	while(bytes_remaining > 0){
		
		if (bytes_remaining < load_mp_buffer_size  ){
			
			current_data_len = bytes_remaining ;
			
		
			current_page_count = bytes_remaining / NAND_PAGE_SIZE ;
		
			u32 p;
			p = current_page_count * NAND_PAGE_SIZE ;
			
			if (p < bytes_remaining ){
				current_page_count ++ ;
			}	
				

		}
		else{
			current_data_len = load_mp_buffer_size ;
			current_page_count = load_mp_buffer_pages ;
		}

		bytes_remaining -= current_data_len;

		rop = begin_read_op(false);
		
		for (u32 i = 0 ; i < current_page_count ; i++ )
		{
			VPRINTF_STARTUP ( "\t %u ppa:[chn:%u,die:%u,plane:%u,blk:%u,p:%u] , tables data addr:0x%x, size:%u, bytes remaining:%u" ,
						 current_pg_index,
						 mapping_tables[current_pg_index].path.chn ,
						 mapping_tables[current_pg_index].path.die ,
						 mapping_tables[current_pg_index].path.plane ,
						 mapping_tables[current_pg_index].path.block,
						 mapping_tables[current_pg_index].path.page,
						 current_map_tables_addr, current_data_len, bytes_remaining) ;

			// add page program operation
			add_read_op_page(rop, mapping_tables[current_pg_index].data, buffer_addr + (i * NAND_PAGE_SIZE ), (void*)0);
			
			current_pg_index ++;
		}

		VPRINTF_STARTUP (" Start reading flash ")
		// do page program
		sync_end_read_op(rop);
		
		// copy data from uncached buffer
		memcpy8(current_map_tables_addr, buffer_addr, current_data_len  );
		// adjust map table address 
		current_map_tables_addr += current_data_len;
					
	}

	VPRINTF_STARTUP ("done");
}


int shutdown_ftl_module(){
	
	store_mapping_tables();
	
	return 0 ;
}







