

#ifndef HOST_SSD_INTERFACE_HEADER
#define HOST_SSD_INTERFACE_HEADER


#include "iSSDHostIntCmdReg.h"


// Circular Queue Common Operations
typedef struct {
	int tail; 
	int head; 
	int size ; 
	void **objs ;
} queue_t ;


struct __attribute__ ((__packed__)) SimulationMeta{
	char 	host_name [256] ;
	u64 	host_start_time ;
    u64     host_started ;
    char    device_name [256];
    u64     device_started ;
    u64     simulation_status ;
} ;
typedef volatile struct SimulationMeta sim_meta_t ;

#define dbUpdateQueueSize (1<<22)
struct __attribute__ ((__packed__)) DoorBellUpdateQueue{
	///queue_t queue ;
	int tail; 
	int head;
	dbUpdateReg list[dbUpdateQueueSize] ;
} ;
typedef volatile struct DoorBellUpdateQueue db_update_queue_t ;

#define SSDMsgQueueSize 			(1<<10)
#define INTERFACE_DMA_PAGE_COUNT 	SSDMsgQueueSize

#define INTERFACE_DMA_PAGE_SIZE 	(1<<16) 

struct __attribute__ ((__packed__)) InterfacePage {
	int        data_status ;
	#define    data_status_ready   1
	#define    data_status_used    2
	#define    data_status_free	   0
	int        page_index ;
	u8 	       data [INTERFACE_DMA_PAGE_SIZE];
};
typedef volatile struct InterfacePage page_reg_t ;

struct __attribute__ ((__packed__)) InterfacePageQueue{
	int head ;
	int tail ;
	page_reg_t 	list[INTERFACE_DMA_PAGE_COUNT] ;
};
typedef volatile struct InterfacePageQueue page_queue_t ;


struct SSD_Message {
	 hostCmdReg registers ;
	 bool 		msg_ready ;
	 int 		page_index;
     int        msg_index ;
} ;
typedef volatile struct SSD_Message ssd_msg_t ;

struct __attribute__ ((__packed__)) SSDMsgQueue{
	int tail; 
	int head;
	ssd_msg_t 	list[SSDMsgQueueSize] ;
} ;
typedef volatile struct SSDMsgQueue ssd_msg_queue_t ;


struct  __attribute__ ((__packed__)) SharedMemoryLayout {	
	volatile  struct SimulationMeta  		sim_meta ;
	struct DoorBellUpdateQueue 	db_queue ;
	struct SSDMsgQueue 			msg_queue;
	struct InterfacePageQueue	page_queue ; 
    nvmeCtrlReg                 nvmeReg;
    char                        DoorBellRegisters[8 * 128];
} ;
typedef volatile struct SharedMemoryLayout shmem_layout_t;


static inline page_reg_t* enQueuePageRegister (page_queue_t *q ){
    page_reg_t* page;

    page = &q->list[0] ;
    page->page_index = 1 ;
    page->data_status = data_status_free ;
    return page ;

    // check if full 
    if (q->head == ((q->tail + 1) % INTERFACE_DMA_PAGE_COUNT ) ) { return NULL ; } 
    // add to items 
    page = &q->list[q->tail] ;
    page->page_index = q->tail + 1 ;
    page->data_status = data_status_free ;
    // adjust the tail pointer
    q->tail = (q->tail+ 1 ) % INTERFACE_DMA_PAGE_COUNT ;
    //fprintf(stdout, "[%s: %u] page index:%d\n", __FUNCTION__ ,__LINE__, page->page_index ); fflush(stdout) ;
    return page ;
}

static inline page_reg_t * deQueuePageRegister  ( page_queue_t * q ){
    // check if empty 
    if (q->head == q->tail){  return NULL ;  }
    // get the item 
     page_reg_t * page  = &q->list[q->head];
    // adjust the head pointer
    q->head = (q->head + 1 ) % INTERFACE_DMA_PAGE_COUNT ;
    //fprintf(stdout, "[%s: %u] page index:%d\n",__FUNCTION__ ,__LINE__, page->page_index ); fflush(stdout) ;
    return page;
}

static inline  page_reg_t* getPageRegister(   page_queue_t* q , int page_index){

	if( page_index <= 0 || page_index > INTERFACE_DMA_PAGE_COUNT ){
		fprintf( stderr  , " page index out of range  %d\n", page_index ); fflush(stderr) ;
		return NULL ;
	}
    //fprintf(stdout, "[%s: %u] page index:%d\n",__FUNCTION__ ,__LINE__, q->list[page_index -1].page_index ); fflush(stdout) ;
	return  & q->list[page_index-1];
}

static inline   ssd_msg_t* enQueueSSDMsg  (   ssd_msg_queue_t *q  , u32 command ){
    
    // check if full 
    if (q->head == ((q->tail + 1) % SSDMsgQueueSize ) ) { return NULL ; } 
    
    // add to items 
         ssd_msg_t* msg = &q->list[q->tail] ;
    msg->registers.cmd  = command;
    msg->msg_ready = false ;
    msg->msg_index = q->tail + 1 ;
   
    // adjust the tail pointer
    q->tail = (q->tail+ 1 ) % SSDMsgQueueSize ;
    //fprintf(stdout, "[%s: %u] msg index:%d\n",__FUNCTION__ ,__LINE__, msg->msg_index ); fflush(stdout) ;
    return msg   ;
}

static inline    ssd_msg_t * deQueueSSDMsg  (  ssd_msg_queue_t * q ){
   
     // check if empty 
    if (q->head == q->tail){  return NULL ;  }
    
    // get the item 
      ssd_msg_t * msg  = &q->list[q->head];
    // adjust the head pointer
    q->head = (q->head + 1 ) % SSDMsgQueueSize ;
    //fprintf(stdout, "[%s: %u] msg index :%d \n", __FUNCTION__ ,__LINE__, msg->msg_index  );
    return msg;
}

static inline    dbUpdateReg* enQueueDBUpdate (   db_update_queue_t *q    ,  dbUpdateReg * item){
    
    // check if full 
    if (q->head == ((q->tail + 1) % dbUpdateQueueSize ) ) { return NULL ; } 
    
    // add to items 
    dbUpdateReg* update = (dbUpdateReg*) &q->list[q->tail] ;
    update->DoorBell = item->DoorBell ;
	update->Qindex =   item->Qindex ;
	update->db_value = item->db_value ;


    // adjust the tail pointer
    q->tail = (q->tail+ 1 ) % dbUpdateQueueSize ;
    
    return update ;
}

static inline   dbUpdateReg * deQueueDBUpdate (  db_update_queue_t * q ){
   
    // check if empty 
    if (q->head == q->tail){  return NULL ;  }
    
    // get the item 
    dbUpdateReg * item  = (dbUpdateReg*) &q->list[q->head];
    // adjust the head pointer
    q->head = (q->head + 1 ) % dbUpdateQueueSize ;

    return item;
}






#endif
