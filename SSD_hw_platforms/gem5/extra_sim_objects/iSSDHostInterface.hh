
//#define print_src_line fprintf(sim_verbose_fd, " %s : %u \n", __FUNCTION__ ,__LINE__ );fflush(sim_verbose_fd);
#define print_src_line 



#ifndef __DEV_ARM_iSSDHostinterface_HH__
#define __DEV_ARM_iSSDHostinterface_HH__

#include <queue>
#include <deque>
#include "base/addr_range.hh"
#include "base/bitfield.hh"
#include "base/statistics.hh"
#include "dev/arm/base_gic.hh"
#include "dev/dma_device.hh"
#include "dev/io_device.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/serialize.hh"
#include "sim/stats.hh"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "params/iSSDHostInterface.hh"
#include "../../../include/issd_types.h"
#include "../../../include/nvmeDataStructures.h"
#include "../../../include/host_ssd_interface.h"


class iSSDHostInterface : public DmaDevice  
{
public:
	typedef iSSDHostInterfaceParams Params;
	
	iSSDHostInterface(const Params *p);
	  ~iSSDHostInterface(){};
	
	/* Host controller information */
	const Addr 			pioAddr;
	const Addr 			pioSize;
	const Tick 			pioDelay; 
	const int 			intNum;						//  Door bell update interrupt number  
	BaseGic 			*gic;						//  Pointer to the GIC for causing an interrupt 
	
	/* Address range functions */
	AddrRangeList 		getAddrRanges() const override;

	/*  register access functions */
	Tick 				read(PacketPtr pkt) override;
	Tick 				write(PacketPtr pkt) override;
	
	Tick 				dmaHostWriteDelay;			//  host dma write latency Tick/Byte 
	Tick 				dmaHostReadDelay;			//  host dma read latency Tick/Byte 

	hostCmdReg 			Registers ;  				// host command registers 
	
	std::string shmem_id ;
	shmem_layout_t *shared_mem ;
	sim_meta_t *sim_meta ;
	db_update_queue_t  *db_queue ;
	ssd_msg_queue_t *msg_queue;
	page_queue_t   *page_queue ; 
	volatile nvmeCtrlReg  *nvmeReg ;

	void connect_shared_memory();
	void start_read_host();	
	void read_host_complete(ssd_msg_t*);
	void write_ssd_complete(ssd_msg_t*);
	
	void start_write_host();
	void read_ssd_complete(ssd_msg_t*);
	void write_host_complete(ssd_msg_t*);

	void enable_msix_on_hostcpu();

	class Event_read_host_complete : public Event
	{
	public:
		iSSDHostInterface * p_owner ;
		ssd_msg_t * p_msg ;
	
		Event_read_host_complete(iSSDHostInterface * owner , ssd_msg_t * msg  ) :
		Event(Default_Pri, AutoDelete), p_owner(owner) , p_msg(msg) { }
		
		// Process the event. Just call into the  owner. 
		void process() override {
			p_owner->read_host_complete(p_msg);
		}
	};

	class Event_write_ssd_complete  : public Event
	{
	public:
		iSSDHostInterface * p_owner ;
		ssd_msg_t * p_msg ;
	
		Event_write_ssd_complete (iSSDHostInterface * owner , ssd_msg_t * msg  ) :
		Event(Default_Pri, AutoDelete), p_owner(owner) , p_msg(msg) { }
		
		// Process the event. Just call into the  owner. 
		void process() override {
			p_owner->write_ssd_complete (p_msg);
		}
	};

	class Event_read_ssd_complete  : public Event
	{
	public:
		iSSDHostInterface * p_owner ;
		ssd_msg_t * p_msg ;
	
		Event_read_ssd_complete (iSSDHostInterface * owner , ssd_msg_t * msg  ) :
		Event(Default_Pri, AutoDelete), p_owner(owner) , p_msg(msg) { }
		
		// Process the event. Just call into the  owner. 
		void process() override {
			p_owner->read_ssd_complete (p_msg);
		}
	};

	class Event_write_host_complete  : public Event
	{
	public:
		iSSDHostInterface * p_owner ;
		ssd_msg_t * p_msg ;
	
		Event_write_host_complete (iSSDHostInterface * owner , ssd_msg_t * msg  ) :
		Event(Default_Pri, AutoDelete), p_owner(owner) , p_msg(msg) { }
		
		// Process the event. Just call into the  owner. 
		void process() override {
			p_owner->write_host_complete (p_msg);
		}
	};
 
	FILE 				*sim_verbose_fd;

};

 
#endif





















