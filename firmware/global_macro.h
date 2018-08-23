 

// Define limits and constants
#define NVME_CTRL_OF 		0x200		// Nvme controller register offset 
#define NUM_NS				1
#define NUM_CHANNELS		nand_num_chn 
#define PKG_PER_CHANNEL		nand_pkgs_per_chn	
#define DIES_PER_PKG		nand_dies_per_pkg 
#define PLANES_PER_DIE 		nand_planes_per_die	
#define BLOCKS_PER_PLANE	nand_blocks_per_plane 
#define PAGES_PER_BLOCK 	nand_pages_per_block	
#define NAND_DS 			nand_page_size 
#define NAND_PAGE_SIZE 		(1<<nand_page_size)
#define HOST_DS 			host_page_size  
#define HOST_PAGE_SIZE 		(1<<host_page_size)


#define DIES_PER_CHANNEL		( PKG_PER_CHANNEL * DIES_PER_PKG )
#define PLANES_PER_CHANNEL		( PKG_PER_CHANNEL * DIES_PER_PKG * PLANES_PER_DIE )
#define PAGES_PER_PLANE 		( BLOCKS_PER_PLANE * PAGES_PER_BLOCK )
#define PAGES_PER_DIE			( PAGES_PER_PLANE * PLANES_PER_DIE )
#define PAGES_PER_PKG			( PAGES_PER_DIE * DIES_PER_PKG )
#define PAGES_PER_CHANNEL		( PAGES_PER_PKG * PKG_PER_CHANNEL )

#define TOTAL_NUM_OF_PKGS		( NUM_CHANNELS * PKG_PER_CHANNEL )
#define TOTAL_NUM_OF_DIES		( TOTAL_NUM_OF_PKGS * DIES_PER_PKG )
#define TOTAL_NUM_OF_PLANES		( TOTAL_NUM_OF_DIES * PLANES_PER_DIE )
#define TOTAL_NUM_OF_BLOCK		( TOTAL_NUM_OF_PLANES * BLOCKS_PER_PLANE )
#define TOTAL_NUM_OF_PAGES		( TOTAL_NUM_OF_BLOCK * PAGES_PER_BLOCK )
#define TOTAL_FLASH_SIZE		( TOTAL_NUM_OF_PAGES * NAND_PAGE_SIZE )
#define HOST_2_NAND_DS 			(NAND_DS - HOST_DS)
#define HOST_PER_NAND_PAGES		(1 << HOST_2_NAND_DS)
#define TOTAL_NUM_OF_HOST_PAGES (TOTAL_NUM_OF_PAGES << HOST_2_NAND_DS )

#define NUM_NAMESPACES 			num_namespaces 
#define OVP 					over_provision_factor 

// limits 
#define MAX_ADMIN_SQCQ_ENTRIES		256			// SSD max entires pair submission and completion queues  	
#define MAX_IO_SQCQ_PAIRS			4			// SSD max submission/completion pairs 	
#define MAX_IO_SQCQ_ENTRIES			128  //4  //	256			// SSD max entires pair submission and completion queues  	
#define INTERNAL_ADMIN_REQ_QUEUE  	16 		 
#define INTERNAL_IO_REQ_QUEUE  		128
#define MAX_PAGES_PER_BLOCK 		256 
#define MAX_NUM_CHANNELS 			64
#define MAX_NUM_NAMESPACES 			4


//(NUM_CHANNELS * PKG_PER_CHANNEL * DIES_PER_PKG  * PLANES_PER_DIE * HOST_PER_NAND_PAGES )

#define MAX_LP_TX 			16	 //16 /* Maximun number of logical pages per host transfer */
#define MAX_TX_SIZE 		(MAX_LP_TX << HOST_DS) /* Maximun data size per host transfer */

#define MDTS_PAGES 			MAX_LP_TX
#define MDTS_SIZE  			MAX_TX_SIZE

#define MDT_PAGES			MAX_LP_TX
#define MDT_SIZE			MAX_TX_SIZE

#define WBC_PAGES			MDTS_PAGES /* MAximum number of logical pages in a write back cache request */

// all request object memory size should not be larger than 1K
#define MAX_REQUEST_OBJ_MEM_SIZE 1024

//  basic threads 
#define HOST_DMA_THREAD				1
#define NVME_DRIVER_THREAD			2
#define FTL_MODULE_THREAD			3
#define NAND_IO_THREAD				20
#define ISP_THREAD_START_INDEX		4

// Request data structure types
#define ADMIN_REQUEST 				1
#define READ_IO_REQUEST 			2
#define WRITE_IO_REQUEST 			3
#define FLUSH_IO_REQUEST			4
#define TRIM_IO_REQUEST 			5
#define ISP_IO_REQUEST				6
#define WBC_FLUSH_REQUEST 			7
// Request types for internal isp request 
#define ISP_READ_PAGES_REQUEST 		10
#define ISP_PROGRAM_PAGES_REQUEST 	11
#define ISP_RESERVE_PAGES_REQUEST 	12
#define ISP_HOST_DMA_REQUEST		13
#define ISP_EXECUTION_EXIT			14


#define FLASH_READ_OPERATION		8
#define FLASH_PROGRAM_OPERATION 	9

// global queues
#define FTL_MODULE_IN_Q				5
#define FTL_MODULE_OUT_Q			6
#define ISP_Q_START_INDEX 	 		7	


// Request stages
#define REQ_STAGE_FREE 		 		1
#define REQ_STAGE_INUSE				2
#define REQ_STAGE_FTL 				3
#define REQ_STAGE_READING_FLASH  	4
#define REQ_STAGE_PROGRAMMING_FLASH 5
#define REQ_STAGE_DMA 		 		6
#define PENDING_CCMD_POST			7
#define REQ_COMPLETED 				8	
#define CCMD_POSTED 				REQ_COMPLETED 

#define ISP_DATA_TRANSFERING		10
#define ISP_CTRL_PENDING_MSG 		11
#define ISP_EXEC_PENDING_POST  		13
#define ISP_HOST_DMA_REQ_INIT		14
#define ISP_HOST_DMA_REQ_TX_ON		15
#define ISP_HOST_DMA_REQ_CMPT		16
#define ISP_NAND_OP_INIT			17
#define ISP_NAND_OP_CMPLT 			18
#define ISP_FOR_HOST_DMA_REQ 		19  



