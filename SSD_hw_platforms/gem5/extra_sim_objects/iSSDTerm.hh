 

#ifndef __DEV_ARM_SSDHostInterface_HH__
#define __DEV_ARM_SSDHostInterface_HH__

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


#include "dev/io_device.hh"
#include "dev/terminal.hh"

#include "../../include/iSSDCommonTypes.h"
#include "params/iSSDTerm.hh"

class Platform;

const int RX_INT = 0x1;
const int TX_INT = 0x2;

 
class iSSDTerm : public DmaDevice  
{
	
protected:
	 
	int status;
	Platform *platform;
	Terminal *term;
	
public:
	typedef iSSDTermParams Params;
	
	iSSDTerm(const Params *p);
	  ~iSSDTerm(){};
	
	/* Host controller information */
	const Addr pioAddr;
	const Addr pioSize;
	const Tick pioDelay; 
	
	const Addr stats_pioAddr ;
	const Addr stats_pioSize;
	Tick stats_updateTick_ltcy;
	void updateTick();
	EventWrapper<iSSDTerm, &iSSDTerm::updateTick> updateTickEvent;
	
	
	/*  interrupt Number to cause door bell value change */
	const int intNum;
	/* Pointer to the GIC for causing an interrupt */
	BaseGic *gic;
	
	/* Address range functions */
	AddrRangeList getAddrRanges() const override;

	/*  register access functions */
	Tick read(PacketPtr pkt) override;
	Tick write(PacketPtr pkt) override;
	
	     
	/**
	 * Inform the uart that there is data available.
	 */
	 void dataAvailable() ;

	
protected:
	MakeCallback< iSSDTerm , & iSSDTerm ::dataAvailable> callbackDataAvail;

protected: // Registers
	static const uint64_t AMBA_ID = ULL(0xb105f00d00341011);
	static const int UART_DR = 0x000;
	static const int UART_FR = 0x018;
	static const int UART_FR_CTS  = 0x001;
	static const int UART_FR_TXFE = 0x080;
	static const int UART_FR_RXFE = 0x010;
	static const int UART_IBRD = 0x024;
	static const int UART_FBRD = 0x028;
	static const int UART_LCRH = 0x02C;
	static const int UART_CR   = 0x030;
	static const int UART_IFLS = 0x034;
	static const int UART_IMSC = 0x038;
	static const int UART_RIS  = 0x03C;
	static const int UART_MIS  = 0x040;
	static const int UART_ICR  = 0x044;
	
	static const uint16_t UART_RIINTR = 1 << 0;
	static const uint16_t UART_CTSINTR = 1 << 1;
	static const uint16_t UART_CDCINTR = 1 << 2;
	static const uint16_t UART_DSRINTR = 1 << 3;
	static const uint16_t UART_RXINTR = 1 << 4;
	static const uint16_t UART_TXINTR = 1 << 5;
	static const uint16_t UART_RTINTR = 1 << 6;
	static const uint16_t UART_FEINTR = 1 << 7;
	static const uint16_t UART_PEINTR = 1 << 8;
	static const uint16_t UART_BEINTR = 1 << 9;
	static const uint16_t UART_OEINTR = 1 << 10;
	
	/* Stats */
	// Stats reporting
	Stats::Scalar cpu_reads ;
	Stats::Scalar cpu_writes ;
	
	void regStats() override ;

};


#endif




















