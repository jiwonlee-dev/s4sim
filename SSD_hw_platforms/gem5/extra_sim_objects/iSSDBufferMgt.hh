/*
 * File: iSSDBufferMgt.hh
 * Date: 2017. 05. 05.
 * Author: Thomas Haywood Dadzie (ekowhaywood@hanyang.ac.kr)
 * Copyright(c)2017 
 * Hanyang University, Seoul, Korea
 * Security And Privacy Laboratory. All right reserved
 */


#ifndef __DEV_ARM_iSSDBufferMgt_HH__
#define __DEV_ARM_iSSDBufferMgt_HH__
 
#include <vector>
#include <queue>
#include <deque>
#include "mem/mem_object.hh"

#include "params/iSSDBufferMgt.hh"

class iSSDBufferMgt : public MemObject
{
	
	
private:
	
	/**
	 * Port on the Nand Controller Side that receives requests.
	 */
	class NandCtrlSide : public SlavePort
	{
	private:
		/// The object that owns this object (SimpleMemobj)
		iSSDBufferMgt *owner;
		
		/// True if the port needs to send a retry req.
		bool needRetry;
		
		/// If we tried to send a packet and it was blocked, store it here
		PacketPtr blockedPacket;
		
	public:
		/**
		 * Constructor. Just calls the superclass constructor.
		 */
		NandCtrlSide(const std::string& name, iSSDBufferMgt *owner) ;
		 
		/**
		 * Send a packet across this port. This is called by the owner and
		 * all of the flow control is hanled in this function.
		 *
		 * @param packet to send.
		 */
		
		void sendPacket(PacketPtr pkt);
		
		PacketPtr Packet ;
		void trysendTimingResp();
		EventWrapper<NandCtrlSide, &NandCtrlSide::trysendTimingResp> trysendTimingRespEvent;
		
		 

		
		/**
		 * Get a list of the non-overlapping address ranges the owner is
		 * responsible for. All slave ports must override this function
		 * and return a populated list with at least one item.
		 *
		 * @return a list of ranges responded to
		 */
		AddrRangeList getAddrRanges() const override;
		
		/**
		 * Send a retry to the peer port only if it is needed. This is called
		 * from the SimpleCache whenever it is unblocked.
		 */
		void trySendRetry();
		
	protected:
		/**
		 * Receive an atomic request packet from the master port.
		 * No need to implement in this simple memobj.
		 */
		Tick recvAtomic(PacketPtr pkt) override
		{ panic("recvAtomic unimpl."); }
		
		/**
		 * Receive a functional request packet from the master port.
		 * Performs a "debug" access updating/reading the data in place.
		 *
		 * @param packet the requestor sent.
		 */
		void recvFunctional(PacketPtr pkt) override;
		
		/**
		 * Receive a timing request from the master port.
		 *
		 * @param the packet that the requestor sent
		 * @return whether this object can consume to packet. If false, we
		 *         will call sendRetry() when we can try to receive this
		 *         request again.
		 */
		bool recvTimingReq(PacketPtr pkt) override;
		
		/**
		 * Called by the master port if sendTimingResp was called on this
		 * slave port (causing recvTimingResp to be called on the master
		 * port) and was unsuccesful.
		 */
		void recvRespRetry() override;
	};
	
	/**
	 * Port on the Host Interface  Side that receives requests.
	 */
	class HostIntSide : public SlavePort
	{
		private:
		/// The object that owns this object (SimpleMemobj)
		iSSDBufferMgt *owner;
		
		/// True if the port needs to send a retry req.
		bool needRetry;
		
		/// If we tried to send a packet and it was blocked, store it here
		PacketPtr blockedPacket;
		
		public:
		/**
		 * Constructor. Just calls the superclass constructor.
		 */
		HostIntSide(const std::string& name, iSSDBufferMgt *owner) ;
		
		/**
		 * Send a packet across this port. This is called by the owner and
		 * all of the flow control is hanled in this function.
		 *
		 * @param packet to send.
		 */
		
		void sendPacket(PacketPtr pkt);
		
		PacketPtr Packet ;
		void trysendTimingResp();
		EventWrapper<HostIntSide, &HostIntSide::trysendTimingResp> trysendTimingRespEvent;
		
		
		
		
		/**
		 * Get a list of the non-overlapping address ranges the owner is
		 * responsible for. All slave ports must override this function
		 * and return a populated list with at least one item.
		 *
		 * @return a list of ranges responded to
		 */
		AddrRangeList getAddrRanges() const override;
		
		/**
		 * Send a retry to the peer port only if it is needed. This is called
		 * from the SimpleCache whenever it is unblocked.
		 */
		void trySendRetry();
		
		protected:
		/**
		 * Receive an atomic request packet from the master port.
		 * No need to implement in this simple memobj.
		 */
		Tick recvAtomic(PacketPtr pkt) override
		{ panic("recvAtomic unimpl."); }
		
		/**
		 * Receive a functional request packet from the master port.
		 * Performs a "debug" access updating/reading the data in place.
		 *
		 * @param packet the requestor sent.
		 */
		void recvFunctional(PacketPtr pkt) override;
		
		/**
		 * Receive a timing request from the master port.
		 *
		 * @param the packet that the requestor sent
		 * @return whether this object can consume to packet. If false, we
		 *         will call sendRetry() when we can try to receive this
		 *         request again.
		 */
		bool recvTimingReq(PacketPtr pkt) override;
		
		/**
		 * Called by the master port if sendTimingResp was called on this
		 * slave port (causing recvTimingResp to be called on the master
		 * port) and was unsuccesful.
		 */
		void recvRespRetry() override;
	};

	
	typedef struct bufferedPage{
		int RW ;
		uint8_t data[512];
	}bufferedPage;
	
	std::queue<bufferedPage *> wbufferQ ;  // write buffer queue
	std::queue<bufferedPage *> rbufferQ ;  // read buffer queue
	
	Addr readBufferSize ;
	Addr writeBufferSize ;
	
	/**
	 * Handle the write request from Nand Controller side 
	 *
	 * @param requesting packet
	 * @return true if data is succesfully placed in read buffer,
	 *         false if read buffer is full, slave port must handle retry with requester 
	 */
	bool handleNCWrite(PacketPtr pkt);
	
	/**
	 * Handle the read request from Nand Controller side 
	 *
	 * @param requesting packet
	 * @return true if data is succesfully placed in packet from write buffer,
	 *         false if write buffer is empty, slave port must handle retry with requester 
	 */
	bool handleNCRead(PacketPtr pkt);

	
	/**
	 * Handle the write request from Host Interface Controller side 
	 *
	 * @param requesting packet
	 * @return true if data is succesfully placed in write buffer,
	 *         false if write buffer is full, slave port must handle retry with requester 
	 */
	bool handleHIntWrite(PacketPtr pkt);
	
	/**
	 * Handle the read request from Host Interface Controller side 
	 *
	 * @param requesting packet
	 * @return true if data is succesfully placed in packet from read buffer,
	 *         false if read buffer is empty, slave port must handle retry with requester 
	 */
	
	
	bool handleHIntRead(PacketPtr pkt);
	
	
	
	bool hasBlockedNCWriteReq ;
	bool hasBlockedNCReadReq ;
	bool hasBlockedHIntReadReq ;
	bool hasBlockedHintWriteReq ;
	
	/**
	 * Handle the respone from the memory side
	 *
	 * @param responding packet
	 * @return true if we can handle the response this cycle, false if the
	 *         responder needs to retry later
	 */
	bool handleResponse(PacketPtr pkt);
	
	/**
	 * Handle a packet functionally. Update the data on a write and get the
	 * data on a read.
	 *
	 * @param packet to functionally handle
	 */
	void handleFunctional(PacketPtr pkt);
	
	/**
	 * Return the address ranges this memobj is responsible for. Just use the
	 * same as the next upper level of the hierarchy.
	 *
	 * @return the address ranges this memobj is responsible for
	 */
	AddrRangeList getAddrRanges() const;
	
	/**
	 * Tell the CPU side to ask for our memory ranges.
	 */
	void sendRangeChange();
	
	/// Instantiation of the CPU-side ports
	NandCtrlSide ncSide;
	HostIntSide  hintSide;
	//CPUSidePort dataPort;
	
	
public:
	
	
	
	/** constructor
	 */
	iSSDBufferMgt(iSSDBufferMgtParams *params);
	
	/**
	 * Get a master port with a given name and index. This is used at
	 * binding time and returns a reference to a protocol-agnostic
	 * base master port.
	 *
	 * @param if_name Port name
	 * @param idx Index in the case of a VectorPort
	 *
	 * @return A reference to the given port
	 */
	BaseMasterPort& getMasterPort(const std::string& if_name,
								  PortID idx = InvalidPortID) override;
	
	/**
	 * Get a slave port with a given name and index. This is used at
	 * binding time and returns a reference to a protocol-agnostic
	 * base master port.
	 *
	 * @param if_name Port name
	 * @param idx Index in the case of a VectorPort
	 *
	 * @return A reference to the given port
	 */
	BaseSlavePort& getSlavePort(const std::string& if_name,
								PortID idx = InvalidPortID) override;
	
	
	
//	/*   True if write buffer is Empty . */
//	bool wBufferEmpty; 
//	/*   True if write buffer is Full . */	
//	bool wBufferFull ;
//	/*   True if read buffer is Empty . */
//	bool rBufferEmpty
//	/*   True if read buffer is Empty . */
//	bool rBufferFull
	
};


#endif // __LEARNING_GEM5_SIMPLE_MEMOBJ_SIMPLE_MEMOBJ_HH__
