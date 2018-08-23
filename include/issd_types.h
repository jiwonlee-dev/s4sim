
#include <stdint.h>
#include <time.h>
#include <stdbool.h>


#ifndef COMMON_DATA_TYPES_TYPES
#define COMMON_DATA_TYPES_TYPES

#define _packed __attribute__ ((__packed__))  
#define __PACKED__ __attribute__ ((__packed__)) 

#define _32bitSystem
#ifdef _32bitSystem
typedef uint32_t sysPtr ;
#define sysByteWidth   4 
#else
typedef uint64_t sysPtr ;
#define sysByteWidth 8
#endif 

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef volatile u32 reg ;

typedef u32 ftn_t ;
#define NULL_FTN 0xFFFF 

typedef union __attribute__ ((__packed__)) __reg64{ 
	uint64_t raw ; 
	struct __attribute__ ((__packed__)) {
		uint32_t lbits ;
		uint32_t hbits ;
	}Dwords;
} reg64;

typedef union __attribute__ ((__packed__)) {
	uint32_t raw;
	
	struct __attribute ((__packed__)){
		uint16_t word_0;
		uint16_t word_1;
	}words;
	
	struct __attribute__ ((__packed__)){
		uint8_t byte_0;
		uint8_t byte_1;
		uint8_t byte_2;
		uint8_t byte_3;
	}bytes;
	
}dword_t ;

// global type for logical and physical page address/number 
typedef uint32_t lpn_t ; 
typedef uint64_t ppa_data_t;
typedef  union __attribute__ ((__packed__)) physical_page_address{
	ppa_data_t		data ;
	reg64			reg ;
	struct {
		uint16_t	page ;
		uint16_t	block ;
		uint8_t		plane;
		uint8_t		die ;
		uint8_t		chn ;
		uint8_t		resv;
	} path ;
}ppa_t ;



enum DeviceCommandStatus { 
	INITCOMPLETE	= 0x00010000, 
	PROCESSING		= 0x00020000, 
	COMPLETE		= 0x00030000,
	HOST_ATTACHED   = 0x00040000,
	HOST_DETTACHED  = 0x00050000
};

enum DeviceIDs{
	DEV_HOST_INT	= 1,
	DEV_NAND_CTRL	= 2,
	DEV_STATS		= 3
};


struct __attribute__ ((__packed__)) Stats_Event_t{	
	
	u32 	event_code ;
	u64 	event_cycle ;

	union __attribute__ ((__packed__)) {
		u64 longs [2];
		u32 ints[4];
		u16 shorts[8];
		u8  chars [16];
	} data ;	
};
typedef struct Stats_Event_t event_t;

typedef struct __attribute__ ((__packed__)) Sim_stats_t{

	u64 	read_io;
	u64 	write_io;
	u64 	flush_io;
	u64 	trim_io;
	u64 	isp_io;


	u64 	tatal_pages;
	u64 	free_pages;
	u64 	valid_pages;
	u64 	invalid_page;

	// reported by nand controller
	u64 	total_nand_pages;
	u64 	free_nand_pages;
	u64 	valid_nand_pages;

	u64 	nand_read;
	u64 	nand_program;
	u64 	nand_erase;


	// reported by wbc 

}sim_stats_t;

#define Event_Queue_Size (1<<17)

struct __attribute__ ((__packed__)) EventLogMemBlock{

	u32 		head ;
	u32 		tail ;
	event_t 	list[Event_Queue_Size];

	sim_stats_t stats ;
};
typedef struct EventLogMemBlock event_queue_t;


// Event Log Codes
#define LOG_READ_IO 			100
#define LOG_WRITE_IO 			101
#define LOG_TRIM_IO 			102
#define LOG_FLUSH_IO 			103
#define LOG_ISP_IO 				104


#define LOG_NAND_READS  		120
#define LOG_NAND_PROGRAMS 		121
#define LOG_NAND_ERASE  		122



#define LOG_ISP_START_TIME		1000
#define LOG_ISP_END_TIME		1001


#endif







