 

#ifndef FTL_MODULE_HEADER 
#define FTL_MODULE_HEADER
 
// Flash Translation Layer Configuration 
// saved in flash memory
typedef struct FTL_Configuration  {
	
	uint32_t 	nand_ds ;
	uint32_t 	host_ds ;
	uint32_t	pages_per_block ;
	uint32_t 	total_pages ;		
	
	uint32_t	rpp_ovp;			// reserved physical pages for over provision purposes
	uint32_t	rpp_map_tables ;	// reserved physical pages for saving translation mapping tables
	uint32_t	total_rpp;			// total reserved pages
	uint32_t	total_logical_pages;// total logical pages accessible by host
	
	uint32_t 	total_invalid_pages ;
	uint32_t 	total_valid_pages	;
	uint32_t 	total_free_pages	;
	
	ppa_t 		last_selected_ppa	;
	
	//ppa_t		mapping_tables;
	ppa_t		mapping_tables[1224];
		
} ftl_config_t  ;

// lpa to ppa mapping table
__attribute__ ((section("mapping_tables")))
extern ppa_t lpa_table[TOTAL_NUM_OF_PAGES];

//ppa to lpa mapping table
__attribute__ ((section("mapping_tables")))
extern lpn_t ppa_table[NUM_CHANNELS][DIES_PER_CHANNEL][PLANES_PER_DIE][BLOCKS_PER_PLANE][PAGES_PER_BLOCK];

__attribute__ ((section("mapping_tables")))
extern block_meta_t blk_table[NUM_CHANNELS] [DIES_PER_CHANNEL] [PLANES_PER_DIE] [BLOCKS_PER_PLANE] ;

// reserved ppa to lpa mapping values
enum ppa_map_state{ 
	FREE_PPA		= 0x40000000 ,	// free physical page 
	INVALID_PPA		= 0x80000000 ,	// stalled physical page 
	RESERVED_PPA	= 0xc0000000 	// ppa is free but it is reserved for 
	// i.	GC process i.e another valid page will be moved here
	// ii.	For storing flash translation table data
};

#define VALID_PPA_MASK	0xc0000000 
#define MAX_LPA_SUPPORTED 0x3FFFFFFF

#define SET_MAPPED_PPA(lpa , ppa )     \
lpa_table[lpa].data = ppa.data  

#define GET_MAPPED_PPA(lpa) \
lpa_table[lpa].data  

#define SET_UNMAPPED_LPA(lpa ) \
lpa_table[lpa].path.chn = 0xFF 

static inline bool IS_UNMAPPED_LPA(lpn_t lpn) {
	return (lpa_table[lpn].path.chn == 0xFF);
}
#define SET_MAPPED_LPA(lpa, ppa) \
ppa_table	[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] = lpa 

#define GET_MAPPED_LPA(ppa) \
(ppa_table	[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] )

#define MAP_LPA_AND_PPA(lpa ,ppa) \
SET_MAPPED_PPA(lpa , ppa  ); \
SET_MAPPED_LPA(lpa , ppa  );

#define SET_FREE_PPA(ppa) \
ppa_table[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane] \
[ppa.path.block] \
[ppa.path.page] = FREE_PPA 

#define IS_FREE_PPA( ppa )		\
(ppa_table[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane] \
[ppa.path.block] \
[ppa.path.page] == FREE_PPA )

#define SET_INVALID_PPA(ppa)	\
ppa_table[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] = INVALID_PPA 

#define IS_INVALID_PPA(ppa)                \
(ppa_table[ppa.path.chn ]           \
[ppa.path.die]             \
[ppa.path.plane]           \
[ppa.path.block]           \
[ppa.path.page] == INVALID_PPA )

#define SET_RESERVED_PPA(ppa)	\
ppa_table[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] = RESERVED_PPA 

#define IS_RESERVED_PPA(ppa)	\
(ppa_table[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] == RESERVED_PPA )

#define IS_VALID_PPA(ppa ) \
(! ((ppa_table	[ppa.path.chn ] \
[ppa.path.die] \
[ppa.path.plane]	\
[ppa.path.block] \
[ppa.path.page] ) & VALID_PPA_MASK))

#define INC_VALID_PAGES(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].valid_pgs ++ 

#define DEC_VALID_PAGES(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].valid_pgs -- 

#define INC_INVALID_PAGES(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].invalid_pgs ++ 

#define DEC_INVALID_PAGES(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].invalid_pgs -- 

#define GET_INVALID_PAGES(ppa)			\
(blk_table  [ppa.path.chn]             \
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].invalid_pgs) 

#define GET_VALID_PAGES(ppa)			\
(blk_table  [ppa.path.chn]             \
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].valid_pgs )

#define GET_FREE_PAGES(ppa)			\
(PAGES_PER_BLOCK				\
-				\
(blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].valid_pgs \
+					\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].invalid_pgs)) 

#define SET_FREE_BLOCK(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].valid_pgs = 0 ;\
\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].invalid_pgs = 0 ; 

#define INC_BLOCK_ERASE_COUNT(ppa)			\
blk_table  [ppa.path.chn]			\
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].erase_count ++ ;\

#define GET_BLOCK_ERASE_COUNT(ppa)			\
(blk_table  [ppa.path.chn]             \
[ppa.path.die]			\
[ppa.path.plane]		\
[ppa.path.block].erase_count )

void initialize_ftl(uint32_t arg1);
void process_req_cbk(void * req , uint32_t req_type );
void store_mapping_tables();
void restore_mapping_tables();

#endif




