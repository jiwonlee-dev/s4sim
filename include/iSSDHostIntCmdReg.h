

#ifndef HOST_INTERFACE_REGISTERS_HEADER
#define HOST_INTERFACE_REGISTERS_HEADER


#define S_CMD_FORMAT_SIZE 64
#define C_CMD_FORMAT_SIZE 16

// Host Interface Commands 
#define	INIT					0x00000001 
#define	dmaABORT			 	0x00000002 
#define	RESET					0x00000003
#define HOST_CONNECTION_STATUS  0x00000004
#define	PCI_DMA_READ			0x00000010
#define	PCI_DMA_WRITE			0x00000020 
#define	PCI_DMA_SQ_READ			0x00000030 
#define	PCI_DMA_CQ_WRITE		0x00000040 
#define	PCI_ENABLE_MSIX_VECTOR	0x00000050 


typedef struct __attribute__ ((__packed__)) dbUpdateReg {
	uint8_t		DoorBell ;
	uint8_t		Qindex ;
	uint16_t	db_value ;
}dbUpdateReg;

#define isSQ_DB  1 
#define isCQ_DB  2

/*	  
	Command Issue Register Set 
 */
typedef struct __attribute__ ((__packed__)) hostCommandRegisters{
	
	reg cmd ;				// write to this register will starts a dma request to host 
	reg cmdStatus;
	reg dbUpdate ;

	// registers for pci_dma_read/write 
	reg64 dma_addr;			// host physical address 
	reg size ;				// dma data size in bytes
	reg buffMgtID ;         // buffID=0	for transferring data between SSD DRAM and Host
							// buffID>=1	for transferring data between SSD Buffer Manager and Host
	reg ssdAddr ;			// SSD memory address
	reg bytesRemaining;		
	
	// registers for sq command read request
	reg squeue ;			// SQ index
	reg sqe ;				// SQ entry index
	reg sqAddr ;			// SSD memory address 
	reg64 sqeDmaAddr;		// host physcal address for submission command 
	
	// registers for cq command write request 
	reg cqueue ;			// CQ index
	reg cqe ;				// CQ entry index
	reg cqAddr ;			// SSD memory address
	reg64 cqeDmaAddr;		// host physcal address for completion command 
	reg cqIntVector ;		// interrupt Vector if interrupt is enable for the cq
	reg isr_notify ;		// 1: if host interfcase must raise interrupt after posting cq command
	 
	// reserved
	reg64 prp1 ;		
	reg64 prp2 ;

}hostCmdReg ;


#endif
