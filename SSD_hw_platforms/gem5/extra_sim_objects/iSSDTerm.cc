
#include <vector>
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


#include "debug/iSSDTerm.hh"
#include "dev/arm/iSSDTerm.hh"
  
int stats_pioAddr_offset ;
stats_output_t  *stats_shm;



iSSDTerm::iSSDTerm(const Params *p):
DmaDevice(p),
platform(p->platform),
term(p->terminal),
pioAddr(p->pio_addr),
pioSize(p->pio_size),
pioDelay(p->pio_latency),
stats_pioAddr(p->stats_pioAddr),
stats_pioSize(p->stats_pioSize),
stats_updateTick_ltcy(p->stats_updateTick_ltcy),
updateTickEvent(this),
intNum(p->int_num),
gic(p->gic),
callbackDataAvail(this)
{
	 
	// setup terminal callbacks
	term->regDataAvailCallback(&callbackDataAvail);
 
	// setup the shared mem for communicating stats 
	//stats_pioAddr_offset = stats_pioAddr - pioAddr ;

//	int shmid;
//	key_t key = 3313;
//		
//	if ((shmid = shmget(key, STATS_SHM_SIZE , 0666|IPC_CREAT)) < 0) {
//		fprintf( stderr , " shmget Error " );
//		
//	}else {
//		/** Now we attach the segment to our data space. */
//		if ((stats_shm = (stats_output_t *)shmat(shmid, NULL, 0)) == (stats_output_t *) -1) {
//			fprintf( stderr , " shmat Error " );
//		}else{
//			memset  (stats_shm, 0 , STATS_SHM_SIZE - 4 );
//		}
//	}
//
//	
//	/*   set the start time */
//	time_t timer;
//	struct tm* tm_info;
//	time(&timer);
//	tm_info = localtime(&timer);
//	memcpy ((void*)&stats_shm->start_tm_info , tm_info , sizeof(tm) );
//	
//	stats_shm->simulated_ticks = curTick() ;
//	schedule(&updateTickEvent, curTick() + stats_updateTick_ltcy );
//	
//	stats_shm->reset = 0 ;
	
	
}

void iSSDTerm::updateTick(){
	
	stats_shm->simulated_ticks = curTick() ;
	
	// re-schedule
	schedule(&updateTickEvent, curTick() + stats_updateTick_ltcy );
	
}

iSSDTerm *
iSSDTermParams::create()
{
	return new iSSDTerm(this);
}

/**
 * Determine address ranges
 */
AddrRangeList
iSSDTerm::getAddrRanges() const
{
	AddrRangeList ranges;
	ranges.push_back(RangeSize(pioAddr, pioSize));
	ranges.push_back(RangeSize( stats_pioAddr , stats_pioSize ) );
	return ranges;
}
 
 
Tick
iSSDTerm::read(PacketPtr pkt)
{
	 
	
	pkt->makeResponse();
	return pioDelay;
}
  



Tick
iSSDTerm::write(PacketPtr pkt)
{

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
			panic("Uart write size too big?\n");
			break;
	}
	
	switch (daddr) {
		case UART_DR:
			//if ((data & 0xFF) == 0x04 && endOnEOT)
			//	exitSimLoop("UART received EOT", 0);
			
			//term->out('t' & 0xFF);
			//term->out('o' & 0xFF);
			//term->out('m' & 0xFF);

			term->out(data & 0xFF);
			
			break;
		
		default :
			
 
			
			break;
	}

	
	 
		
	pkt->makeResponse();
	return pioDelay;
}


void
iSSDTerm::dataAvailable()
{
	/*@todo ignore the fifo, just say we have data now
	 * We might want to fix this, or we might not care */
	DPRINTF(iSSDTerm , "Data available, scheduling interrupt\n");
	//raiseInterrupts(UART_RXINTR | UART_RTINTR);
}


void
iSSDTerm::regStats(){
	using namespace Stats;
	
	cpu_reads
	.name(name() + ".cpu_reads")
	.desc("Number of read request from CPU")
	.flags(total)
	;
	
	
	cpu_writes
	.name(name() + ".cpu_writes")
	.desc("Number of write request from CPU")
	.flags(total)
	;
	
}

  
