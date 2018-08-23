
#ifndef NVME_DATA_STRUCTURES_HEADER
#define NVME_DATA_STRUCTURES_HEADER

#include "issd_types.h"

/* status code types */
enum nvme_status_code_type {
	NVME_SCT_GENERIC			= 0x0,
	NVME_SCT_COMMAND_SPECIFIC	= 0x1,
	NVME_SCT_MEDIA_ERROR		= 0x2,
	/* 0x3-0x6 - reserved */
	NVME_SCT_VENDOR_SPECIFIC	= 0x7,
	
};

/* generic command status codes */
enum nvme_generic_command_status_code {
	NVME_SC_SUCCESS						= 0x00,
	NVME_SC_INVALID_OPCODE				= 0x01,
	NVME_SC_INVALID_FIELD				= 0x02,
	NVME_SC_COMMAND_ID_CONFLICT			= 0x03,
	NVME_SC_DATA_TRANSFER_ERROR			= 0x04,
	NVME_SC_ABORTED_POWER_LOSS			= 0x05,
	NVME_SC_INTERNAL_DEVICE_ERROR		= 0x06,
	NVME_SC_ABORTED_BY_REQUEST			= 0x07,
	NVME_SC_ABORTED_SQ_DELETION			= 0x08,
	NVME_SC_ABORTED_FAILED_FUSED		= 0x09,
	NVME_SC_ABORTED_MISSING_FUSED		= 0x0a,
	NVME_SC_INVALID_NAMESPACE_OR_FORMAT	= 0x0b,
	NVME_SC_COMMAND_SEQUENCE_ERROR		= 0x0c,
	
	NVME_SC_LBA_OUT_OF_RANGE			= 0x80,
	NVME_SC_CAPACITY_EXCEEDED			= 0x81,
	NVME_SC_NAMESPACE_NOT_READY			= 0x82,
	NVME_SC_DNR							= 0x4000,
};

/* command specific status codes */
enum nvme_command_specific_status_code {
	NVME_SC_COMPLETION_QUEUE_INVALID			= 0x00,
	NVME_SC_INVALID_QUEUE_IDENTIFIER			= 0x01,
	NVME_SC_MAXIMUM_QUEUE_SIZE_EXCEEDED			= 0x02,
	NVME_SC_ABORT_COMMAND_LIMIT_EXCEEDED		= 0x03,
	/* 0x04 - reserved */
	NVME_SC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED	= 0x05,
	NVME_SC_INVALID_FIRMWARE_SLOT				= 0x06,
	NVME_SC_INVALID_FIRMWARE_IMAGE				= 0x07,
	NVME_SC_INVALID_INTERRUPT_VECTOR			= 0x08,
	NVME_SC_INVALID_LOG_PAGE					= 0x09,
	NVME_SC_INVALID_FORMAT						= 0x0a,
	NVME_SC_FIRMWARE_REQUIRES_RESET				= 0x0b,
	
	NVME_SC_CONFLICTING_ATTRIBUTES			= 0x80,
	NVME_SC_INVALID_PROTECTION_INFO			= 0x81,
	NVME_SC_ATTEMPTED_WRITE_TO_RO_PAGE		= 0x82,
};

/* media error status codes */
enum nvme_media_error_status_code {
	NVME_SC_WRITE_FAULTS					= 0x80,
	NVME_SC_UNRECOVERED_READ_ERROR			= 0x81,
	NVME_SC_GUARD_CHECK_ERROR				= 0x82,
	NVME_SC_APPLICATION_TAG_CHECK_ERROR		= 0x83,
	NVME_SC_REFERENCE_TAG_CHECK_ERROR		= 0x84,
	NVME_SC_COMPARE_FAILURE					= 0x85,
	NVME_SC_ACCESS_DENIED					= 0x86,
};

/* admin opcodes */
enum nvme_admin_opcode {
	NVME_OPC_DELETE_IO_SQ				= 0x00,
	NVME_OPC_CREATE_IO_SQ				= 0x01,
	NVME_OPC_GET_LOG_PAGE				= 0x02,
	/* 0x03 - reserved */
	NVME_OPC_DELETE_IO_CQ				= 0x04,
	NVME_OPC_CREATE_IO_CQ				= 0x05,
	NVME_OPC_IDENTIFY					= 0x06,
	/* 0x07 - reserved */
	NVME_OPC_ABORT						= 0x08,
	NVME_OPC_SET_FEATURES				= 0x09,
	NVME_OPC_GET_FEATURES				= 0x0a,
	/* 0x0b - reserved */
	NVME_OPC_ASYNC_EVENT_REQUEST		= 0x0c,
	/* 0x0d-0x0f - reserved */
	NVME_OPC_FIRMWARE_ACTIVATE			= 0x10,
	NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD	= 0x11,
	
	NVME_OPC_FORMAT_NVM					= 0x80,
	NVME_OPC_SECURITY_SEND				= 0x81,
	NVME_OPC_SECURITY_RECEIVE			= 0x82,
};

/* nvme nvm opcodes */
enum nvme_nvm_opcode {
	NVME_OPC_FLUSH					= 0x00,
	NVME_OPC_WRITE					= 0x01,
	NVME_OPC_READ					= 0x02,
	/* 0x03 - reserved */
	NVME_OPC_WRITE_UNCORRECTABLE	= 0x04,
	NVME_OPC_COMPARE				= 0x05,
	/* 0x06-0x07 - reserved */
	NVME_OPC_DATASET_MANAGEMENT		= 0x09,
};


union _packed cap_lo_register {
	uint32_t	raw;
	struct _packed    {
		/** maximum queue entries supported */
		uint32_t mqes		: 16;
		
		/** contiguous queues required */
		uint32_t cqr		: 1;
		
		/** arbitration mechanism supported */
		uint32_t ams		: 2;
		
		uint32_t reserved1	: 5;
		
		/** timeout */
		uint32_t to			: 8;
	} bits ;
} ;

union _packed cap_hi_register {
	uint32_t	raw;
	struct _packed {
		/** doorbell stride */
		uint32_t dstrd			: 4;
		
		uint32_t reserved3		: 1;
		
		/** command sets supported */
		uint32_t css_nvm		: 1;
		
		uint32_t css_reserved	: 3;
		uint32_t reserved2		: 7;
		
		/** memory page size minimum */
		uint32_t mpsmin			: 4;
		
		/** memory page size maximum */
		uint32_t mpsmax			: 4;
		
		uint32_t reserved1		: 8;
	} bits  ;
}  ;

union _packed cc_register {
	uint32_t	raw;
	struct  _packed {
		/** enable */
		uint32_t en			: 1;
		
		uint32_t reserved1	: 3;
		
		/** i/o command set selected */
		uint32_t css		: 3;
		
		/** memory page size */
		uint32_t mps		: 4;
		
		/** arbitration mechanism selected */
		uint32_t ams		: 3;
		
		/** shutdown notification */
		uint32_t shn		: 2;
		
		/** i/o submission queue entry size */
		uint32_t iosqes		: 4;
		
		/** i/o completion queue entry size */
		uint32_t iocqes		: 4;
		
		uint32_t reserved2	: 8;
	} bits  ;
}  ;

union  _packed csts_register {
	uint32_t	raw;
	struct  _packed {
		/** ready */
		uint32_t rdy		: 1;
		
		/** controller fatal status */
		uint32_t cfs		: 1;
		
		/** shutdown status */
		uint32_t shst		: 2;
		
		uint32_t reserved1	: 28;
	} bits  ;
}  ;

union  _packed aqa_register {
	uint32_t	raw;
	struct  _packed {
		/** admin submission queue size */
		uint32_t asqs		: 12;
		
		uint32_t reserved1	: 4;
		
		/** admin completion queue size */
		uint32_t acqs		: 12;
		
		uint32_t reserved2	: 4;
	} bits  ;
}  ;
 

typedef struct  _packed nvme_Controller_Registers
{
	/** controller capabilities */
	union cap_lo_register		cap_lo;
	union cap_hi_register		cap_hi;
	
	uint32_t				vs;		/* version */
	uint32_t				intms;	/* interrupt mask set */
	uint32_t				intmc;	/* interrupt mask clear */
	
	union cc_register			cc;		/** controller configuration */
	
	uint32_t				reserved1;
	
	union csts_register		csts;	/** controller status */
	
	uint32_t				reserved2;
	
	union aqa_register		aqa;	/** admin queue attributes */
	
	reg64					asq;	/* admin submission queue base addr */
	reg64					acq;	/* admin completion queue base addr */
	uint32_t				reserved3[0x3f2];
	
	/** Door Bell Registers */
	uint32_t				sq_tdbl; /* Admin submission queue tail doorbell */
	uint32_t				cq_hdbl; /* Admin completion queue head doorbell */
	
	
} nvmeCtrlReg    ;

#define AdminDBTail_OFFSET 0x1000
#define AdminDBHead_OFFSET 0x1004

typedef union __attribute__ ((__packed__)) __Dword { 
	uint32_t raw ; 
	struct __attribute__ ((__packed__)) {
		uint16_t w0 ;
		uint16_t w1	;
	}words;
	
	struct __attribute__ ((__packed__)) {
		uint8_t b0 ;
		uint8_t b1 ;
		uint8_t b2 ;
		uint8_t b3 ;
	}bytes;
	
} Dword;


union  _packed cdw_10_11 {
	
	uint64_t raw;
	
	// words 
	struct _packed {
		Dword cdw10 ;
		Dword cdw11 ;
	} Dwords ;
	
	// create i/o submission queue 
	struct  _packed {
		/** queue identifier  */
		uint32_t sqid		: 16;
		/** queue size */
		uint32_t size		: 16;
		/** Physically Contiguous */
		uint32_t pc			: 1;
		/** Queue Priority */
		uint32_t qprio		: 2;
		/** reserved */
		uint32_t reserved1	: 13;
		/** Completion Queue Identifier */
		uint32_t cqid		: 16;
		
	} createIOSQ ;
	
	// create i/o completion queue 
	struct  _packed {
		/** queue identifier  */
		uint32_t cqid		: 16;
		/** queue size */
		uint32_t size		: 16;
		/** Physically Contiguous */
		uint32_t pc			: 1;
		/** Interrupt Enable */
		uint32_t ien		: 1;
		/** reserved */
		uint32_t reserved1	: 14;
		/** Interrupt Vector  */
		uint32_t ivector 	: 16;
		
	} createIOCQ ;
	
	struct _packed {
		uint32_t cns		: 2;
		uint32_t reserved	: 30;
		uint32_t reserved1	   ;
	} identify ;
	
	// dsm command 
	struct _packed{
		uint8_t nr ;			/* number of ranges , This is a 0’s based value */
		uint8_t reserved1;
		uint16_t reserved2;
		
		uint32_t idr		: 1 ;
		uint32_t idw		: 1 ;
		uint32_t ad			: 1 ;
		uint32_t reserved3	: 29 ;
		
	}dsm ;
	
	// fw download , activate 
	struct _packed {
		uint32_t num_of_Dwords ;
		uint32_t Dword_offset ;
	} fw_downlaod ;
	
	struct _packed {
		uint32_t firmware_slot	 : 3;
		uint32_t activate_action : 2;
		uint32_t reserved1		 : 27;
		uint32_t reserved2 ;
	}fw_activate ;
	
}  ;

// NVMe Command Structures 

typedef struct _packed NvmeSubmissionCommand {
	// generic nvme command fields
	uint8_t     opcode;		// opcode : read, write or flush 
	uint8_t     fuse;		// fused operation 2 bits , rsvd1 6 bits 
	uint16_t    cid;		// command identifier 
	uint32_t    nsid;		// namespace identifier 
	uint64_t    resv1;		// reserved field
	uint64_t    mptr;		// metadata pointer 
	uint64_t    prp1;		// First page address 
	uint64_t    prp2;		// Second page address or PRP list address 
	uint32_t    cdw10;		// command specific 
	uint32_t    cdw11;		// command specific 
	uint32_t    cdw12;		// command specific 
	uint32_t    cdw13;		// command specific 
	uint32_t    cdw14;		// command specific 
	uint32_t    cdw15;		// command specific 
} NvmeSCmd;

typedef struct _packed NvmeCompletionCommand {
	// generic nvme command fields
	uint32_t	cdw0;		// command-specific 
	uint32_t	rsvd1;		// reserved
	uint16_t	sqhd;		// submission queue head pointer 
	uint16_t	sqid;		// submission queue identifier 
	uint16_t	cid;		// command identifier 
	uint16_t	status_p;	// status code and phase tag 
} NvmeCCmd;


typedef struct _packed NvmeIdentifyCmd {
	uint8_t     opcode;		
	uint8_t     fuse;		
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;		
	uint32_t	cns		: 2;		// Controller or Namespace Structure
	uint32_t 	resv1 	: 30;
	uint32_t 	resv2 	   ;
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmeIdentifyCmd ;

typedef struct _packed NvmeSetFeatureCmd {
	uint8_t     opcode;		
	uint8_t     fuse;		
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;		
	uint32_t	fid		: 8;		// Feature Identifier 
	uint32_t 	resv1 	: 24;
	uint16_t 	nsq  ;			//  Number of Submission Queues to Set  
	uint16_t 	ncq  ;			//  Number of Completion Queues to Set
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmeSetFeatureCmd ;
typedef NvmeSetFeatureCmd NvmeGetFeatureCmd ;

typedef struct _packed NvmeCreateSQCmd {
	uint8_t     opcode;		
	uint8_t     fuse;		
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;		
	uint32_t 	sqid		: 16; 	// queue identifier  
	uint32_t 	size		: 16;	// queue size 
	uint32_t 	pc			: 1; 	// Physically Contiguous 
	uint32_t 	qprio		: 2;		// Queue Priority 
	uint32_t 	reserved1	: 13; 	// reserved 
	uint32_t 	cqid		: 16; 	// Completion Queue Identifier 
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmeCreateSQCmd ;
 
typedef struct _packed NvmeCreateCQCmd {
	uint8_t     opcode;		
	uint8_t     fuse;		
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;		
	uint32_t 	cqid		: 16; 	// queue identifier  
	uint32_t 	size		: 16;	// queue size 
	uint32_t 	pc			: 1;		// Physically Contiguous 
	uint32_t 	ien			: 1;		// Interrupt Enable 
	uint32_t 	reserved1	: 14;	// reserved 
	uint32_t 	ivector 	: 16;	// Interrupt Vector  
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmeCreateCQCmd ;


typedef struct _packed NvmeFirmwareDownloadCmd {
	uint8_t     opcode;		
	uint8_t     fuse;		
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;	
	uint32_t 	num_dwords ; // Number of DWords contains in the portion of the firmware image being downloaded  
	uint32_t 	dword_off ; // DWord offset from 0 (the start) associated with this firmware piece
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmefwdlCmd ;

typedef struct _packed NvmeFirmwareActivateCmd {
	uint8_t     opcode;		
	uint8_t     fuse;
	uint16_t    cid;		
	uint32_t    nsid;		
	uint64_t    res1;
	uint64_t    mptr;		
	uint64_t    prp1;		
	uint64_t    prp2;	
	uint32_t 	fs		: 3; // Firmware Slot (FS) – Field used by AA field to indicate which slot to be updated and/or activated
	uint32_t 	aa 	 	: 2; // Active Action (AA) – Action taken on the downloaded image or image associated with a firmware slot
	uint32_t 	resv1 		: 27;
	uint32_t 	resv2 ;
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmefwActCmd ;

// IO command fields for read, write, flush and trim 
typedef struct _packed  NvmeIOSubmissionCommand{ 
	uint8_t     opcode;			 
	uint8_t     fuse;			 
	uint16_t    cid;			 
	uint32_t    nsid;			
	uint64_t    res1;
	uint64_t    mptr;			
	uint64_t    prp1;			
	uint64_t    prp2;			
	uint64_t	slba;			// Address of first logical block to read 
	uint16_t	nlbs	;		//  Number of logical blocks to read from NVM 
	uint32_t	rsvd1	: 10;	 
	uint32_t	prinfo 	: 4 ;	//  Protection Information Field  
	uint32_t	fua 	: 1 ;	//  Force Unit Access 
	uint32_t	lr		: 1 ;	//  Limited Retry 	
	uint8_t		dsm			;	
	uint32_t	rsvd2	: 24;
	uint32_t	refTag;			// Expected Initial Logical Block Reference Tag 
	uint16_t	appTagMask;		// Expected Logical Block Application Tag Mask 
	uint16_t	appTag;			// Expected Logical Block Application Tag	
}NvmeIOSCmd ;

typedef struct _packed  NvmeDataSetMgtCommand{ 
	uint8_t     opcode;			 
	uint8_t     fuse;			 
	uint16_t    cid;			 
	uint32_t    nsid;			
	uint64_t    res1;
	uint64_t    mptr;			
	uint64_t    prp1;			
	uint64_t    prp2;			
	uint8_t 	nr ;			// Number of Ranges (NR): Indicates the number of range sets, This is a 0’s based value.
	uint8_t 	resv1;
	uint16_t 	resv2;
	uint32_t 	idr		: 1 ;	// Attribute – Integral Dataset for Read (IDR) 
	uint32_t 	idw		: 1 ;	// Attribute – Integral Dataset for Write (IDW) 
	uint32_t 	ad		: 1 ;	// Attribute – Deallocate (AD): If set to ‘1’ , SSD may deallocate all provided ranges.
	uint32_t 	resv3	: 29 ;
	uint32_t    cdw12;		  
	uint32_t    cdw13;		 
	uint32_t    cdw14;		 
	uint32_t    cdw15;		  
}NvmeDsmSCmd ;


struct _packed nvme_power_state {
	/** Maximum Power */
	uint16_t	mp;					/* Maximum Power */
	uint8_t		ps_rsvd1;
	uint8_t		mps      : 1;		/* Max Power Scale */
	uint8_t		nops     : 1;		/* Non-Operational State */
	uint8_t		ps_rsvd2 : 6;
	uint32_t	enlat;				/* Entry Latency */
	uint32_t	exlat;				/* Exit Latency */
	uint8_t		rrt      : 5;		/* Relative Read Throughput */
	uint8_t		ps_rsvd3 : 3;
	uint8_t		rrl      : 5;		/* Relative Read Latency */
	uint8_t		ps_rsvd4 : 3;
	uint8_t		rwt      : 5;		/* Relative Write Throughput */
	uint8_t		ps_rsvd5 : 3;
	uint8_t		rwl      : 5;		/* Relative Write Latency */
	uint8_t		ps_rsvd6 : 3;
	uint16_t	idlp;				/* Idle Power */
	uint8_t		ps_rsvd7 : 6;
	uint8_t		ips      : 2;		/* Idle Power Scale */
	uint8_t		ps_rsvd8;
	uint16_t	actp;				/* Active Power */
	uint8_t		apw      : 3;		/* Active Power Workload */
	uint8_t		ps_rsvd9 : 3;
	uint8_t		aps      : 2;		/* Active Power Scale */
	uint8_t		ps_rsvd10[9];
}  ;

#define NVME_SERIAL_NUMBER_LENGTH	20
#define NVME_MODEL_NUMBER_LENGTH	40
#define NVME_FIRMWARE_REVISION_LENGTH	8

typedef struct _packed nvme_controller_data {
	
	/* bytes 0-255: controller capabilities and features */
	
	/** pci vendor id */
	uint16_t		vid;
	
	/** pci subsystem vendor id */
	uint16_t		ssvid;
	
	/** serial number */
	uint8_t			sn[NVME_SERIAL_NUMBER_LENGTH];
	
	/** model number */
	uint8_t			mn[NVME_MODEL_NUMBER_LENGTH];
	
	/** firmware revision */
	uint8_t			fr[NVME_FIRMWARE_REVISION_LENGTH];
	
	/** recommended arbitration burst */
	uint8_t			rab;
	
	/** ieee oui identifier */
	uint8_t			ieee[3];
	
	/** multi-interface capabilities */
	uint8_t			mic;
	
	/** maximum data transfer size */
	uint8_t			mdts;
	
	uint8_t			reserved1[178];
	
	/* bytes 256-511: admin command set attributes */
	
	/** optional admin command support */
	struct _packed {
		/* supports security send/receive commands */
		uint16_t	security  : 1;
		
		/* supports format nvm command */
		uint16_t	format    : 1;
		
		/* supports firmware activate/download commands */
		uint16_t	firmware  : 1;
		
		uint16_t	oacs_rsvd : 13;
	}   oacs;
	
	/** abort command limit */
	uint8_t			acl;
	
	/** asynchronous event request limit */
	uint8_t			aerl;
	
	/** firmware updates */
	struct _packed {
		/* first slot is read-only */
		uint8_t		slot1_ro  : 1;
		
		/* number of firmware slots */
		uint8_t		num_slots : 3;
		
		uint8_t		frmw_rsvd : 4;
	}   frmw;
	
	/** log page attributes */
	struct _packed {
		/* per namespace smart/health log page */
		uint8_t		ns_smart : 1;
		
		uint8_t		lpa_rsvd : 7;
	}   lpa;
	
	/** error log page entries */
	uint8_t			elpe;
	
	/** number of power states supported */
	uint8_t			npss;
	
	/** admin vendor specific command configuration */
	struct _packed {
		/* admin vendor specific commands use spec format */
		uint8_t		spec_format : 1;
		
		uint8_t		avscc_rsvd  : 7;
	}   avscc;
	
	uint8_t			reserved2[247];
	
	/* bytes 512-703: nvm command set attributes */
	
	/** submission queue entry size */
	struct _packed {
		uint8_t		min : 4;
		uint8_t		max : 4;
	}   sqes;
	
	/** completion queue entry size */
	struct _packed {
		uint8_t		min : 4;
		uint8_t		max : 4;
	}   cqes;
	
	uint8_t			reserved3[2];
	
	/** number of namespaces */
	uint32_t		nn;
	
	/** optional nvm command support */
	struct _packed  {
		uint16_t	compare : 1;
		uint16_t	write_unc : 1;
		uint16_t	dsm: 1;
		uint16_t	reserved: 13;
	}   oncs;
	
	/** fused operation support */
	uint16_t		fuses;
	
	/** format nvm attributes */
	uint8_t			fna;
	
	/** volatile write cache */
	struct  _packed {
		uint8_t		present : 1;
		uint8_t		reserved : 7;
	}   vwc;
	
	/* TODO: flesh out remaining nvm command set attributes */
	uint8_t			reserved4[178];
	
	/* bytes 704-2047: i/o command set attributes */
	uint8_t			reserved5[1344];
	
	/* bytes 2048-3071: power state descriptors */
	struct nvme_power_state power_state[32];
	
	/* bytes 3072-4095: vendor specific */
	uint8_t			vs[1024];
} nvme_controller_data   ;




typedef struct _packed NvmePSD {
	uint16_t    mp;
	uint16_t    reserved;
	uint32_t    enlat;
	uint32_t    exlat;
	uint8_t     rrt;
	uint8_t     rrl;
	uint8_t     rwt;
	uint8_t     rwl;
	uint8_t     resv[16];
} NvmePSD;


typedef struct _packed NvmeIdController {
	uint16_t    vid;
	uint16_t    ssvid;
	uint8_t     sn[20];
	uint8_t     mn[40];
	uint8_t     fr[8];
	uint8_t     rab;
	uint8_t     ieee[3];
	uint8_t     cmic;
	uint8_t     mdts;
	uint8_t     rsvd255[178];
	uint16_t    oacs;
	uint8_t     acl;
	uint8_t     aerl;
	uint8_t     frmw;
	uint8_t     lpa;
	uint8_t     elpe;
	uint8_t     npss;
	uint8_t     rsvd511[248];
	uint8_t     sqes;
	uint8_t     cqes;
	uint16_t    rsvd515;
	uint32_t    nn;
	uint16_t    oncs;
	uint16_t    fuses;
	uint8_t     fna;
	uint8_t     vwc;
	uint16_t    awun;
	uint16_t    awupf;
	uint8_t     rsvd703[174];
	uint8_t     rsvd2047[1344];
	NvmePSD     psd[32];
	uint8_t     vs[1024];
	
} NvmeIdController;
typedef NvmeIdController NvmeIdCtrl ;

typedef struct _packed NvmeLBAF {
	uint16_t    ms;
	uint8_t     ds;
	uint8_t     rp;
} NvmeLBAF;

typedef struct _packed NvmeIdNameSpace {
	uint64_t    nsze;
	uint64_t    ncap;
	uint64_t    nuse;
	uint8_t     nsfeat;
	uint8_t     nlbaf;
	uint8_t     flbas;
	uint8_t     mc;
	uint8_t     dpc;
	uint8_t     dps;
	uint8_t     res30[98];
	NvmeLBAF    lbaf[16];
	uint8_t     res192[192];
	uint8_t     vs[3712];
} NvmeIdNameSpace;
typedef NvmeIdNameSpace NvmeIdNs ;


typedef struct _packed dsm_lba_range{
	
	struct _packed {
		uint32_t ca		; /* bytes 03:00 : Context Attributes */
		uint32_t len	; /* bytes 07:04 : Length in logical blocks */
		uint64_t slba	; /* bytes 15:08 : Starting LBA */
	} ranges[256];
	
}dsm_lba_range;

#endif
