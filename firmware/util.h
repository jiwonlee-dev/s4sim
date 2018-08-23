 


/*
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>*/



#ifndef UTILITY_FUNCTIONS_HEADER  
#define UTILITY_FUNCTIONS_HEADER

#include "verbose_macro.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define __DELAY__(n)		for (int _DelayCounter_ = 0 ; _DelayCounter_ < n ; _DelayCounter_ ++) {} 
 

 
// Circular Queue Common Operations
typedef struct {
	int tail; 
	int head; 
	int size ; 
	void **objs ;
} queue_t ;

#define CONCAT_MACRO(a,b) a ## b

#define DEFINE_CIRCULAR_QUEUE(queue_name , item_size ) \
void* CONCAT_MACRO(queue_name,_items)  [item_size] ; \
queue_t queue_name = {0,0,(int)item_size, &  ( CONCAT_MACRO(queue_name,_items) [0] ) } ;

void* enQueue ( queue_t *queue  , void * item_ptr);
void * deQueue (queue_t * queue ); 


// Printing  to the UART device
#define printbuff_size 1024
#define snprintf_ simple_snprintf
extern char *cpu_string[4] ;
extern char *io_opcode_string[11] ;
extern char *request_type_string[32];
extern char *request_stage_string[32]; 
#define NOT_SET_OPCODE 10 
extern void init_printing();
extern char *get_buff();
extern void queue_print();
extern int simple_snprintf(char *buf, size_t len , char *fmt, ...);
extern char *simple_strcat(char *dest, const char *src);
extern void _sync_print_(char * string_buffer) ;

extern reg * regData ;
extern reg regVar ;
extern char printbuffer[4096];
void memset32(uint32_t * , uint32_t value , size_t count );
void memset8(uint8_t * ptr  , uint8_t value , size_t count );
void memcpy_sysbitw (unsigned char* dst, unsigned char* src, size_t count );
void memcpy8(uint8_t * dst, uint8_t * src, size_t count );
void strpadcpy(char *buf, int buf_size, const char *str, char pad);
extern int DelayCounter ;


extern volatile u32 *SimEventLog;
#define LOG_EVENT(Source,EventCode)  *(SimEventLog + (Source * sizeof(event_t))) = EventCode ; 

#endif




