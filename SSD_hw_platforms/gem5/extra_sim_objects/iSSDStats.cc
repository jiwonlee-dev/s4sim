
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "base/chunk_generator.hh"
#include "debug/Drain.hh"
#include "sim/system.hh"
#include "cpu/simple/timing.hh"
#include "mem/cache/cache.hh"

#include "iSSDStats.hh"

//stats_output_t * stats_dev = 0 ;

pthread_t write_event_log_thrd;

int nsleep(long ); 
int nsleep(long miliseconds){
   struct timespec req, rem;
   if(miliseconds > 999){   
        req.tv_sec = (int)(miliseconds / 1000);                            /* Must be Non-Negative */
        req.tv_nsec = (miliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
   } else {   
        req.tv_sec = 0;                         /* Must be Non-Negative */
        req.tv_nsec = miliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
   }   
   return nanosleep(&req , &rem);
}

int read_ini_file(const char * , char *);
int read_ini_file(const char * iniFile , char *value_buffer){

	FILE 	*sim_verbose_fd = stdout ;
	FILE 	*iniFile_fd;
   
    if (! (iniFile_fd = fopen (iniFile , "r"))){
        fprintf(sim_verbose_fd, "Error openning ini file {%s} \n", iniFile );
        fflush(sim_verbose_fd)  ; 
        return -1 ; 
    }

    int len = fscanf (iniFile_fd , "%s" , value_buffer ) ;

    fclose (iniFile_fd);

    fprintf(sim_verbose_fd, " Reading ini file:%s [%s]\n", iniFile , value_buffer);
    fflush(sim_verbose_fd);

    return len ;
}

int setup_shared_memory(const char *, void ** , u64 );
int setup_shared_memory(const char *iniFile , void **pointer , u64 shmem_size){

	FILE 	*sim_verbose_fd = stdout ;
    //FILE    *shmem_id_fd ;
    char    shmem_name [1<<10] ;
    //char    shmem_id_file [1024] = "sim_out/shared_memory_ids.ini" ;
    int     shmem_fd ;
    void*     shmem_ptr ;
    //size_t  shmem_size ;
    FILE 	*iniFile_fd ;

    sim_verbose_fd = stdout ;// stderr ;
    shmem_ptr = NULL ;

    //shmem_size = sizeof(struct SharedMemoryLayout);

    fprintf(sim_verbose_fd, " Reading shared memory name from %s file \n", iniFile );
    fflush(sim_verbose_fd);
    
    if (! (iniFile_fd = fopen (iniFile , "r"))){
        fprintf(sim_verbose_fd, "Error openning shared memory ID file {%s} \n", iniFile );
        fflush(sim_verbose_fd)  ; 
        return -1 ; 
    }

    int len = fscanf (iniFile_fd , "%s" , shmem_name ) ;

    fprintf(sim_verbose_fd, " Shared memory name:%s, name lenth:%d\n", shmem_name , len );
	fflush(sim_verbose_fd);

    if(((shmem_fd = shm_open( shmem_name  , O_RDWR | O_CREAT  , 0666 ))>=0)){
               
             if((ftruncate(shmem_fd , shmem_size )) == -1){
                fprintf(sim_verbose_fd , "[%s] ftruncate error, shmem_fd:%d errno(%d):%s \n",__FUNCTION__,shmem_fd , errno, strerror(errno));
                fflush(sim_verbose_fd);
                return -1 ;
             }        
                 
             shmem_ptr = mmap(0, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmem_fd, 0);
             if (! shmem_ptr ){
                fprintf(sim_verbose_fd , "[%s] mmap error, shmem_id:%d errno(%d):%s \n",__FUNCTION__,shmem_fd , errno, strerror(errno));
                fflush(sim_verbose_fd);
                return -1 ;
             }
    }else{
       fprintf(sim_verbose_fd , "[%s] shmem error, shmem:%d errno(%d):%s \n",__FUNCTION__,shmem_fd , errno, strerror(errno));
       fflush(sim_verbose_fd);
       return -1 ;
    }
     
    fprintf(sim_verbose_fd , "[%s] shared memory attached successfuly :%d, ptr:%p size:%lu \n",__FUNCTION__,shmem_fd, shmem_ptr , shmem_size );
    fflush(sim_verbose_fd);

    *pointer = shmem_ptr ;
    //memset (shmem_ptr  , 0 , sizeof(shmem_layout_t));print_src_line
   

    return 0 ;
}


static inline event_t* enQueueEvt  ( event_queue_t *q , event_t * e ){
    
    // check if full 
    if (q->head == ((q->tail + 1) % Event_Queue_Size ) ) { return NULL ; } 
    
    // add to items 
    event_t* evt = (event_t*) &q->list[q->tail] ;
    evt->event_code = e->event_code ;
    evt->event_cycle = e->event_cycle;
    evt->data.longs[0] = e->data.longs[0];
	evt->data.longs[1] = e->data.longs[1];

    // adjust the tail pointer
    q->tail = (q->tail+ 1 ) % Event_Queue_Size ;
    
    return evt ;
}

static inline event_t * deQueueEvt ( event_queue_t * q ){
   
    // check if empty 
    if (q->head == q->tail){  return NULL ;  }
    
    // get the item 
    event_t * item  = (event_t*) &q->list[q->head];
    // adjust the head pointer
    q->head = (q->head + 1 ) % Event_Queue_Size ;

    return item;
}


void iSSDStats::addEvent(Stats_Event_t * evt ){
	enQueueEvt ( event_queue , evt );
}

char file_name[1024]; 
u64 writes = 0 ;
event_t wait_buffer[100];
//int wait_buffer_count = 0 ;

size_t flushtofile();
size_t flushtofile(){
	
	FILE 	*fd ;
	size_t 	len ;
	
	fprintf(stderr, "[%s] flushing %lu logs  \n", __FUNCTION__, writes  );
			

	if (!(fd = fopen(file_name , "a+"))){
		fprintf(stdout , "[%s] error opening file :%s errno(%d):%s \n",__FUNCTION__, file_name , errno, strerror(errno));
	}

	len = fwrite ( wait_buffer , sizeof (event_t), writes , fd );

	fclose (fd) ;

	writes = 0 ;

	return len ;
}

void* write_event_log_to_file(void *) ;
void* write_event_log_to_file( void * obj ){

	event_t *evt;
	FILE 	*fd ;
	event_queue_t *event_queue = (event_queue_t*)obj ; 
	
	// open log file
	
	if (read_ini_file("sim_out/event_log_file.ini", file_name) <= 0){ 
		        
		fprintf(stderr, " Error read_ini_file \n" );
	}

	if (!(fd = fopen(file_name , "w+"))){
		fprintf(stdout , "[%s] error opening file :%s errno(%d):%s \n",__FUNCTION__, file_name , errno, strerror(errno));
	}
	fclose(fd) ;


	u64 idle_cycles = 0 ;
	
	while(1){

		if((evt = deQueueEvt(event_queue))){
			
			//fprintf(stderr, "[%s] write log code:%u  \n", __FUNCTION__, evt->event_code  );
			wait_buffer[writes].event_code = evt->event_code ; // fwrite (evt , sizeof (event_t), 1 , fd ); 
			wait_buffer[writes].event_cycle = evt->event_cycle ;
			wait_buffer[writes].data.longs[0] = evt->data.longs[0] ;
			wait_buffer[writes].data.longs[1] = evt->data.longs[1] ;
			writes ++ ;

			if (writes >= 10){
				flushtofile(); // fflush(fd);
				//fprintf(stderr, "[%s] fflush log writes :%lu  \n", __FUNCTION__, writes);
				//writes = 0 ;
			}
			
			idle_cycles = 0 ;
		
		}else{
			
			nsleep(50);
			
			idle_cycles ++ ;
			
			if(idle_cycles > 200 && (writes)){
				flushtofile() ; // fflush(fd);
				//fprintf(stderr, "[%s] fflusdh log writes:%lu  \n", __FUNCTION__, writes   );
				idle_cycles = 0 ;
				//writes = 0 ;
			}
		}

	}

}

 
iSSDStats::iSSDStats(const Params *p):
DmaDevice(p),
pioAddr(p->pio_addr),
pioSize(p->pio_size),
pioDelay(p->pio_latency),
gic(p->gic),
stats_updateTick_ltcy(p->stats_updateTick_ltcy),
updateTickEvent(this)
{
	/*
	int shmid;
	key_t key = 3313;
	
	if ((shmid = shmget(key, STATS_SHM_SIZE , 0666|IPC_CREAT)) < 0) {
		fprintf( stderr , " shmget Error " );
		
	}else {
		// Now we attach the segment to our data space. 
		if ((stats_shm = (uint8_t *)shmat(shmid, NULL, 0)) == (uint8_t *) -1) {
			fprintf( stderr , " shmat Error " );
		}else{
			
		}
	}
	
	memset  (stats_shm, 0 , STATS_SHM_SIZE );
	//   set the start time 
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	stats_output_t * stats_output = (stats_output_t *) stats_shm ;
	memcpy ((void*)&(stats_output->start_tm_info) , tm_info , sizeof(tm) );
	
	stats_output->simulated_ticks = curTick() ;
	schedule(&updateTickEvent, curTick() + stats_updateTick_ltcy );
	
	stats_output->reset = 1 ;
	*/

	setup_shared_memory("sim_out/event_log_shared_memory.ini", (void**)&event_queue , sizeof(event_queue_t) );  

	pthread_create(& write_event_log_thrd  , NULL, write_event_log_to_file , (void*)event_queue);


}


void iSSDStats::updateTick(){
	//((stats_output_t* )stats_shm)->simulated_ticks = curTick() ;	
	// re-schedule
	////schedule(&updateTickEvent, curTick() + stats_updateTick_ltcy );
}


iSSDStats *iSSDStatsParams::create()
{
	return new iSSDStats(this);
}

/** Determine address ranges */
AddrRangeList iSSDStats::getAddrRanges() const
{
	AddrRangeList ranges;
	ranges.push_back(RangeSize(pioAddr, pioSize));
	return ranges;
}

/** This function allows the system to read the register entries */
Tick iSSDStats::read(PacketPtr pkt){
	pkt->makeResponse();
	return pioDelay;
}

/**  This function allows access to the writeable registers.  */
Tick iSSDStats::write(PacketPtr pkt){
	
	Addr daddr = pkt->getAddr() - pioAddr;
	uint32_t data = 0;
	
	switch(pkt->getSize()) {
		case 1:
			data = pkt->get<uint8_t>();
			break;
		case 2:
			data = pkt->get<uint16_t>();
			break;
		case 4:
			data = pkt->get<uint32_t>();
			break;
		default:
			//panic("stats write size too big?\n");
			break;
	}
	
	Addr offset = daddr % sizeof(event_t);
	u64 core_id = daddr / sizeof(event_t);
	if (core_id > 8) core_id = 0;
	//fprintf(stderr, "[%s] daddr:%lu offset:%lu data:%u cpu:%lu \n", __FUNCTION__ , daddr , offset, data , core_id );

	event_t* use_event  = & event[core_id] ;
	memcpy ( use_event + offset , &data , pkt->getSize());
	
	if (offset == 0){
		use_event->event_cycle = curTick() ;
		addEvent(use_event);
	}
	
	pkt->makeResponse();
	return pioDelay;
	
}
  
  



