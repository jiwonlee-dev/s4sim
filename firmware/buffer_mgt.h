

#ifndef BUFFER_MANAGER_HEADER 
#define BUFFER_MANAGER_HEADER

typedef struct read_buffer {
	uint32_t id ;
	sysPtr ptr ; //[MDTS_PAGES] [HOST_PAGE_SIZE] ;	
}r_buffer_t; 

extern void init_ftn_cmd_mgt(unsigned int cpuID);

#endif
