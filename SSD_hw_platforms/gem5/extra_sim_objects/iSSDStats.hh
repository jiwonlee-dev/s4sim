

#ifndef __DEV_ARM_iSSDStats_HH__
#define __DEV_ARM_iSSDStats_HH__

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

#include "params/iSSDStats.hh"
#include "../../../include/issd_types.h"

class iSSDStats : public DmaDevice  
{
public:
	typedef iSSDStatsParams Params;
	
	iSSDStats(const Params *p);
	  ~iSSDStats(){};
	
	/* Host controller information */
	const Addr pioAddr;
	const Addr pioSize;
	const Tick pioDelay; 
	BaseGic *gic;
	event_t  event[8] ;	
	event_queue_t *event_queue ;	
	
	Tick stats_updateTick_ltcy;
	void updateTick();
	EventWrapper<iSSDStats, &iSSDStats::updateTick> updateTickEvent;
	
	/* Address range functions */
	AddrRangeList getAddrRanges() const override;

	/*  register access functions */
	Tick read(PacketPtr pkt) override;
	Tick write(PacketPtr pkt) override;

	void addEvent(event_t * sim_event);
	
private :	
	uint8_t  *stats_shm;
	
};


#endif




















