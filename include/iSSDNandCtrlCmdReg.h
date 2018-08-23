

#ifndef iSSD_NAND_ONTROLLER_REGISTERS_HEADER
#define iSSD_NAND_ONTROLLER_REGISTERS_HEADER

#include "issd_types.h"


enum Commands { 
	INIT_NAND_CTRL		= 0x00000010,
	FLASH_READ			= 0x00000001, 
	FLASH_WRITE			= 0x00000002,
	FLASH_ERASE_BLK		= 0x00000003,
	NAND_CTRL_SHUTDOWN  = 0x00000004
	 
};



// BLOCK_FTL
typedef  union __attribute__ ((__packed__)) physical_block_address{
	
	ppa_data_t		data ;
	
	reg64			reg ;
	
	struct {
		uint32_t	block ;
		uint8_t		plane;
		uint8_t		die ;
		uint8_t		chn ;
		uint8_t		resv;
	} path ;
	
}pba_t ;

// block metadata type
typedef struct __attribute__ ((__packed__)) block_meta_t {
	uint16_t invalid_pgs ;
	uint16_t valid_pgs ;
	//uint16_t free_pgs ;
	uint32_t erase_count ;
}block_meta_t ;


// block metadata type for Block FTL
typedef struct __attribute__ ((__packed__)) block_status_t {
	uint16_t invalid_pgs ;
	uint16_t valid_pgs ;
	uint32_t erase_count ;
	pba_t	parent ;
	pba_t	rp_block1 ;
	pba_t	rp_block2 ;
	uint32_t n_rp_blocks;
	char bitmap[MAX_PAGES_PER_BLOCK];
}block_status_t ;

typedef struct __attribute__ ((__packed__)) nandCtrlRegisters{
	
	reg cmd ;			//  write to this register to start a flash operation 
	reg cmdStatus;		
	reg ssdAddr ;		//  SSD internal memory address
	reg64 PBA ; //ppa_t PBA;			//  physical block address 
	reg size ;			//  size (in bytes) of data to read or write
	
	reg bytesCopied ;	
	
	reg raiseInterrupt ;  
	reg IntNum ;	   
	
	reg64 imgSize	;	//  size ok img file
	
	reg pageListAddr ;		//  SSD internal memory address for list of pages
	reg pageListEntries;    // Page list entries
	
	
}nandCtrlReg ;


typedef struct __attribute__ ((__packed__)) PageOperation {
	
	uint32_t	page ;
	uint64_t	ppa ;
	uint32_t	bmIndex ;   // Buffer Manager index 
	sysPtr		ramAddr ;	// SSD DRAM address if bmIndex is 0  
	
}PageOperation;

typedef struct __attribute__ ((__packed__)) PageOPList  {
	
	PageOperation * pop ;
	uint32_t nop ;
	
}PageOPList ;


typedef struct __attribute__ ((__packed__)) NAND_CHANNEL_CONFIG {
	
	uint8_t pkgs ;
	uint8_t ds ;      
	uint16_t dies ;
	uint32_t planes;
	uint32_t planeblks;
	uint32_t blockpgs;
	uint32_t chnBufferSize ;
	
}NAND_CHANNEL_CONFIG ;

/*
	Nand controller configuration data structure
 */
typedef struct __attribute__ ((__packed__)) NAND_CTRL_CONFIG {
	
	uint32_t numChn;	
	
	NAND_CHANNEL_CONFIG chnConfig [MAX_NUM_CHANNELS] ;

}NAND_CTRL_CONFIG ;



// =============

#define CHANNEL_0_BASE_ADDR		1024
#define CHANNEL_1_BASE_ADDR		2048

#define CHIP_SELECT_REG			0
#define CMD_PHASE_CMD_REG			4
#define ADDRESS_CYCLE_REG_1		8
#define ADDRESS_CYCLE_REG_2		12
  
#define DMA_PHASE_CMD_REG			16
#define DMA_RAM_PTR				20

#define CHIP_STATUS				24
#define WAITING_ROOM_CMD_COUNT		28
#define ACTIVE_OPERATION_COUNT		32

enum NAND_OPERATION{
	
	READ_NAND		= 0x80003000,
	PROGRAM_NAND	= 0x80001080,
	ERASE_NAND		= 0x8000D060,
	
	READ_DMA		= 0x00000001,
	WRITE_DMA		= 0x00000002
};


typedef union __attribute__ ((__packed__)) command_t{
 
	uint32_t data  ;
	
	struct {
		uint32_t    resv					: 31;
		uint32_t	cmd_type				: 1 ;
	}cmd_type;
	
	struct {
		uint8_t		nand_start_cmd;
		uint8_t		nand_end_cmd;
		uint8_t		nand_address_cycles;
		uint32_t	address_cycles_required : 1 ;
		uint32_t	resv					: 2 ;
		uint32_t	cmd_type				: 1 ;
	}cmd_phase;
	
	struct {
		uint8_t		dma_type;
		uint32_t	resv					: 23 ;
		uint32_t	cmd_type				: 1 ;		
	}dma_phase;
}command_t;

typedef union  __attribute__ ((__packed__)) address_cycle_t{
	
	uint64_t data ;
	
	struct {
		uint32_t addr_1;
		uint32_t addr_2;
	}addr ;
	
	struct {
		uint8_t byte;
	}cycle[8] ;
	
	
	struct {
		uint16_t short_1;
		uint16_t short_2;
		uint16_t short_3;
		uint16_t short_4;
	}shorts; 

	
}address_cycle_t ;


#endif






