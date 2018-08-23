/*
 * Copyright (c) 2017 Jason Lowe-Power
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jason Lowe-Power
 */

#include "iSSDBufferMgt.hh"
#include "debug/iSSDBufferMgt.hh"

iSSDBufferMgt::iSSDBufferMgt(iSSDBufferMgtParams *params) :
MemObject(params),
ncSide(params->name + ".nand_ctrl_side", this),
hintSide(params->name + ".host_ctrl_side", this)
//dataPort(params->name + ".data_port", this),
//memPort(params->name + ".mem_side", this),
//blocked(false)
{
	DPRINTF(iSSDBufferMgt , " Object Created");
	readBufferSize = 32768;
	writeBufferSize = 32768;
	
	hasBlockedHIntReadReq = false;
	hasBlockedHintWriteReq = false;
	hasBlockedNCReadReq = false;
	hasBlockedNCWriteReq = false ;
	
}

BaseMasterPort&
iSSDBufferMgt::getMasterPort(const std::string& if_name, PortID idx)
{
//	panic_if(idx != InvalidPortID, "This object doesn't support vector ports");
//	
//	// This is the name from the Python SimObject declaration (SimpleMemobj.py)
//	if (if_name == "mem_side") {
//		return memPort;
//	} else {
//		// pass it along to our super class
		return MemObject::getMasterPort(if_name, idx);
//	}
}

BaseSlavePort&
iSSDBufferMgt::getSlavePort(const std::string& if_name, PortID idx)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	// This is the name from the Python SimObject declaration in iSSD.py
	if (if_name == "ncSide") {
		return ncSide;
	} else if (if_name == "hintSide") {
		return hintSide;
	}  
	
	return  ncSide  ;
}

void
iSSDBufferMgt::NandCtrlSide::sendPacket(PacketPtr pkt)
{
	// Note: This flow control is very simple since the memobj is blocking.
	
//	panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
//	
//	// If we can't send the packet across the port, store it for later.
//	if (!sendTimingResp(pkt)) {
//		blockedPacket = pkt;
//	}
}

AddrRangeList
iSSDBufferMgt::NandCtrlSide::getAddrRanges() const
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	
	AddrRangeList ranges;
	ranges.push_back(RangeSize(0, 8));
	return ranges;

	
	//return owner->getAddrRanges();
}

void
iSSDBufferMgt::NandCtrlSide::trySendRetry()
{
	
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
//	if (needRetry && blockedPacket == nullptr) {
//		// Only send a retry if the port is now completely free
//		needRetry = false;
//		DPRINTF(SimpleMemobj, "Sending retry req for %d\n", id);
		sendRetryReq();
//	}
}

void
iSSDBufferMgt::NandCtrlSide::recvFunctional(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	// Just forward to the memobj.
	//return owner->handleFunctional(pkt);
}
 
void
iSSDBufferMgt::NandCtrlSide::trysendTimingResp(){
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	Packet->makeResponse();
	sendTimingResp(Packet) ;

} 


/*
 
 RequestPtr req = new Request () ;
 req->setPaddr(0);
  
 PacketPtr pkt = Packet::createRead(req);
 
*/

bool
iSSDBufferMgt::NandCtrlSide::recvTimingReq(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	Packet = pkt;
	
	if (pkt->isRead()){
		// Reading write buffer 
		 
		if(owner->handleNCRead(pkt)){
			//  
			return true ;
		}
		
	}else if (pkt->isWrite()){
		
		
		// writing to read buffer 
		if(owner->handleNCWrite(pkt)){
			// schedule response 
			return true ;
		}
	
	}
	
	
		
	return false;
}

void
iSSDBufferMgt::NandCtrlSide::recvRespRetry()
{
	
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
//	// We should have a blocked packet if this function is called.
//	assert(blockedPacket != nullptr);
//	
//	// Grab the blocked packet.
//	PacketPtr pkt = blockedPacket;
//	blockedPacket = nullptr;
//	
//	// Try to resend it. It's possible that it fails again.
//	sendPacket(pkt);
}
//
//void
//iSSDBufferMgt::MemSidePort::sendPacket(PacketPtr pkt)
//{
//	// Note: This flow control is very simple since the memobj is blocking.
//	
////	panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
////	
////	// If we can't send the packet across the port, store it for later.
////	if (!sendTimingReq(pkt)) {
////		blockedPacket = pkt;
////	}
//}
//
//bool
//iSSDBufferMgt::MemSidePort::recvTimingResp(PacketPtr pkt)
//{
//	// Just forward to the memobj.
//	//return owner->handleResponse(pkt);
//}
//
//void
//iSSDBufferMgt::MemSidePort::recvReqRetry()
//{
////	// We should have a blocked packet if this function is called.
////	assert(blockedPacket != nullptr);
////	
////	// Grab the blocked packet.
////	PacketPtr pkt = blockedPacket;
////	blockedPacket = nullptr;
////	
////	// Try to resend it. It's possible that it fails again.
////	sendPacket(pkt);
//}
//
//void
//iSSDBufferMgt::MemSidePort::recvRangeChange()
//{
//	//owner->sendRangeChange();
//}




void
iSSDBufferMgt::HostIntSide::sendPacket(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	// Note: This flow control is very simple since the memobj is blocking.
	
	//	panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
	//	
	//	// If we can't send the packet across the port, store it for later.
	//	if (!sendTimingResp(pkt)) {
	//		blockedPacket = pkt;
	//	}
}

AddrRangeList
iSSDBufferMgt::HostIntSide::getAddrRanges() const
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	AddrRangeList ranges;
	ranges.push_back(RangeSize(0, 8));
	return ranges;
	
	
	//return owner->getAddrRanges();
}

void
iSSDBufferMgt::HostIntSide::trySendRetry()
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	//	if (needRetry && blockedPacket == nullptr) {
	//		// Only send a retry if the port is now completely free
	//		needRetry = false;
	//		DPRINTF(SimpleMemobj, "Sending retry req for %d\n", id);
	sendRetryReq();
	//	}
}

void
iSSDBufferMgt::HostIntSide::recvFunctional(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	// Just forward to the memobj.
	//return owner->handleFunctional(pkt);
}

void
iSSDBufferMgt::HostIntSide::trysendTimingResp(){
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	Packet->makeResponse();
	sendTimingResp(Packet) ;
	
} 


/*
 
 RequestPtr req = new Request () ;
 req->setPaddr(0);
 
 PacketPtr pkt = Packet::createRead(req);
 
 */

bool
iSSDBufferMgt::HostIntSide::recvTimingReq(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	Packet = pkt;
	
	if (pkt->isRead()){
		// Reading write buffer 
		
		if(owner->handleHIntRead(pkt)){
			//  
			return true ;
		}
		
	}else if (pkt->isWrite()){
		// writing to read buffer 
		if(owner->handleHIntWrite(pkt)){
			
			return true ;
		}
		
	}
	
	
	
	return false;
}

void
iSSDBufferMgt::HostIntSide::recvRespRetry()
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	//	// We should have a blocked packet if this function is called.
	//	assert(blockedPacket != nullptr);
	//	
	//	// Grab the blocked packet.
	//	PacketPtr pkt = blockedPacket;
	//	blockedPacket = nullptr;
	//	
	//	// Try to resend it. It's possible that it fails again.
	//	sendPacket(pkt);
}



/**
 * Handle the write request from Nand Controller side 
 *
 * @param requesting packet
 * @return true if data is succesfully placed in read buffer,
 *         false if read buffer is full, slave port must handle retry with requester 
 */
bool iSSDBufferMgt::handleNCWrite(PacketPtr pkt){
	
	
	DPRINTF( iSSDBufferMgt , " NFC write "  );
	
	
	if(rbufferQ.size() >= readBufferSize){
		hasBlockedNCWriteReq = true;
		
		DPRINTF( iSSDBufferMgt , " rbufferQ.size() >= readBufferSize "  );
		
		
		return false ;
		
	}
	
	uint8_t* ptr = pkt->getPtr<uint8_t>();
	DPRINTF (iSSDBufferMgt , " Write From NC : Ptr:%lu \n" , (uint64_t ) ptr );

	bufferedPage * pg = new bufferedPage();
	
	 
	memcpy ( pg->data , ptr , 512 );
	
	DPRINTF ( iSSDBufferMgt , "  data copied  \n") ;
	
	
	//pkt->writeDataToBlock(pg->data , 512);
	
	rbufferQ.push(pg);
	
	hasBlockedNCWriteReq = false ;
	
	schedule(&ncSide.trysendTimingRespEvent , curTick() + 500);
	
	
	if (hasBlockedHIntReadReq){
		hintSide.trySendRetry();
	}

	
	return true ;
}

/*
 
 void 	setDataFromBlock (const uint8_t *blk_data, int blkSize)
 Copy data into the packet from the provided block pointer, which is aligned to the given block size. 
 void 	writeData (uint8_t *p) const
 Copy data from the packet to the provided block pointer, which is aligned to the given block size. 
 void 	writeDataToBlock (uint8_t *blk_data, int blkSize) const
 Copy data from the packet to the memory at the provided pointer. 
  
 */


/**
 * Handle the read request from Nand Controller side 
 *
 * @param requesting packet
 * @return true if data is succesfully placed in packet from write buffer,
 *         false if write buffer is empty, slave port must handle retry with requester 
 */
bool iSSDBufferMgt::handleNCRead(PacketPtr pkt){
		
	DPRINTF( iSSDBufferMgt , " NFC read \n"  );
	
	
	if (wbufferQ.empty())
	{
		hasBlockedNCReadReq = true;
		
		DPRINTF( iSSDBufferMgt , " wbufferQ.empty() \n"  );
		
		
		return false ;
		
	} 
	
	
	bufferedPage * pg = wbufferQ.front() ;
 
	uint8_t* ptr = pkt->getPtr<uint8_t>();
	DPRINTF (iSSDBufferMgt , " read From NC : Ptr:%lu \n" , (uint64_t ) ptr );
	
	memcpy (ptr , pg->data , 512 );
	
	DPRINTF ( iSSDBufferMgt , "  data copied  \n") ;
	
	
	//pkt->setDataFromBlock (pg->data , 512);
	
	wbufferQ.pop();
	
	hasBlockedNCReadReq = false ;
	
	schedule(&ncSide.trysendTimingRespEvent , curTick() + 500);
	
	 
	if (hasBlockedHintWriteReq){
		hintSide.trySendRetry();
	}

	
	return true ;
}


/**
 * Handle the write request from Host Interface Controller side 
 *
 * @param requesting packet
 * @return true if data is succesfully placed in write buffer,
 *         false if write buffer is full, slave port must handle retry with requester 
 */
bool iSSDBufferMgt::handleHIntWrite(PacketPtr pkt){
	
	DPRINTF( iSSDBufferMgt , " H int write \n"  );
	
	
	if(wbufferQ.size() >= writeBufferSize){
		hasBlockedHintWriteReq = true;
		
		DPRINTF( iSSDBufferMgt , " wbufferQ.size() >= writeBufferSize \n"  );
		
		
		
		return false ;
		
	}
	
	
	uint8_t* ptr = pkt->getPtr<uint8_t>();
	DPRINTF (iSSDBufferMgt , " Write From Hint : Ptr:%lu \n" , (uint64_t ) ptr );
	
	bufferedPage * pg = new bufferedPage();
	
	
	memcpy ( pg->data , ptr , 512 );
	
	DPRINTF ( iSSDBufferMgt , "  data copied  \n") ;
	
	
	//pkt->writeDataToBlock(pg->data , 512);
	
	//rbufferQ.push(pg);
	
	
	//bufferedPage * pg = new bufferedPage();
	
	//pkt->writeDataToBlock(pg->data , 512);
	
	wbufferQ.push(pg);
	
	hasBlockedHintWriteReq = false ;
	
	schedule(&hintSide.trysendTimingRespEvent , curTick() + 500);
	
	
	if (hasBlockedNCReadReq){
		ncSide.trySendRetry();
	}
	

	return true ;
}

/**
 * Handle the read request from Host Interface Controller side 
 *
 * @param requesting packet
 * @return true if data is succesfully placed in packet from read buffer,
 *         false if read buffer is empty, slave port must handle retry with requester 
 */
bool iSSDBufferMgt::handleHIntRead(PacketPtr pkt){
	
	DPRINTF( iSSDBufferMgt , " H int read  \n "  );
	
	
	if (rbufferQ.empty())
	{
		hasBlockedHIntReadReq = true;
		
		DPRINTF( iSSDBufferMgt , " rbufferQ.empty() \n"  );
		
		
		return false ;
		
	} 
	
	bufferedPage * pg = rbufferQ.front() ;
 
	 
	uint8_t* ptr = pkt->getPtr<uint8_t>();
	DPRINTF (iSSDBufferMgt , " read From Hint : Ptr:%lu \n" , (uint64_t ) ptr );
	
	memcpy (ptr , pg->data , 512 );
	
	DPRINTF ( iSSDBufferMgt , "  data copied  \n") ;
	
	
	
		
	//pkt->setDataFromBlock (pg->data , 512);
		
		
	rbufferQ.pop();
		
		
	hasBlockedHIntReadReq = false ;
		
		
	schedule(&hintSide.trysendTimingRespEvent , curTick() + 500);
		
		
	
	if (hasBlockedNCWriteReq){
		ncSide.trySendRetry();
	}
	
	 
	return true ;
}


bool
iSSDBufferMgt::handleResponse(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
//	assert(blocked);
//	DPRINTF(SimpleMemobj, "Got response for addr %#x\n", pkt->getAddr());
//	
//	// The packet is now done. We're about to put it in the port, no need for
//	// this object to continue to stall.
//	// We need to free the resource before sending the packet in case the CPU
//	// tries to send another request immediately (e.g., in the same callchain).
//	blocked = false;
//	
//	// Simply forward to the memory port
//	if (pkt->req->isInstFetch()) {
//		instPort.sendPacket(pkt);
//	} else {
//		dataPort.sendPacket(pkt);
//	}
//	
//	// For each of the cpu ports, if it needs to send a retry, it should do it
//	// now since this memory object may be unblocked now.
//	instPort.trySendRetry();
//	dataPort.trySendRetry();
	
	return true;
}

void
iSSDBufferMgt::handleFunctional(PacketPtr pkt)
{
	DPRINTF ( iSSDBufferMgt ,  " %s \n" ,  __FUNCTION__  );
	
	// Just pass this on to the memory side to handle for now.
	//memPort.sendFunctional(pkt);
}

iSSDBufferMgt::HostIntSide::HostIntSide(const std::string& name, iSSDBufferMgt *owner) :
SlavePort(name, owner), owner(owner), needRetry(false),
blockedPacket(nullptr),
trysendTimingRespEvent(this)

{ 
	//DPRINTF(iSSDBufferMgt , " HostIntSide : Object Created");
}

iSSDBufferMgt::NandCtrlSide::NandCtrlSide(const std::string& name, iSSDBufferMgt *owner) :
SlavePort(name, owner), owner(owner), needRetry(false),
blockedPacket(nullptr),
trysendTimingRespEvent(this)
{ 
	//DPRINTF(iSSDBufferMgt , " NandCtrlSide : Object Created");
}

//AddrRangeList
//iSSDBufferMgt::getAddrRanges() const
//{
//	//DPRINTF(SimpleMemobj, "Sending new ranges\n");
//	// Just use the same ranges as whatever is on the memory side.
//	return memPort.getAddrRanges();
//}

//void
//iSSDBufferMgt::sendRangeChange()
//{
//	//instPort.sendRangeChange();
//	//dataPort.sendRangeChange();
//}



iSSDBufferMgt*
iSSDBufferMgtParams::create()
{
	return new iSSDBufferMgt(this);
}
