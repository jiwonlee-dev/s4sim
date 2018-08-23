
from m5.params import *
from m5.proxy import *
from Device import DmaDevice
from SimpleMemory import SimpleMemory
from MemObject import MemObject
from m5.params import *
from m5.proxy import *
from Device import BasicPioDevice, PioDevice, IsaFake, BadAddr, DmaDevice
from Pci import PciConfigAll
from Ethernet import NSGigE, IGbE_igb, IGbE_e1000
from Ide import *
from Platform import Platform
from Terminal import Terminal
from Uart import Uart
from SimpleMemory import SimpleMemory
from Gic import *
from EnergyCtrl import EnergyCtrl


class iSSDHostInterface(DmaDevice):
	type = 'iSSDHostInterface'
	cxx_header = "iSSDHostInterface.hh"
	pio_addr = Param.Addr("")
	pio_size = Param.Addr('0xfff' , "Size")
	dma = MasterPort("SSDHostInterface DMA port")
	pio_latency = Param.Latency("10ns", "Time between action and write/read ")
	gic = Param.BaseGic(Parent.any, "Gic for raising interrupt ")
	int_num = Param.UInt32("Interrupt number that connects to GIC")
	dmaBufferSize = Param.Addr(0x4000 , "buffer size for moving data between host memory and SSD internal memory")
	maxQueueEntries = Param.UInt32(1024,"Controller maximum queue entries supported")
	dmaHostWriteDelay = Param.Latency("20ns" , "latency for host memory write operation per host dma copy buffer size")
	dmaHostReadDelay = Param.Latency("20ns" , "latency for host memory read operation per host dma copy buffer size")
	dmaBufferSize2 = Param.Addr(0x1000 , "buffer size for moving data between host memory and SSD internal memory")
	checkDoorBell_latency = Param.Latency("30ns", "latency")
	buffPort = MasterPort("")
	shmem_id = Param.String("s4sim_id","Shared memory id for Host/SSD communication")


class iSSDNandCtrl(DmaDevice):
	type = 'iSSDNandCtrl'
	cxx_header = "iSSDNandCtrl.hh"
	pio_addr = Param.Addr("Address for SCSI configuration slave interface")
	pio_size = Param.Addr( "Size")
	dma = MasterPort("iSSDNandCtrl DMA port")
	pio_latency = Param.Latency("10ns", "Time between action and write/read")
	gic = Param.BaseGic(Parent.any, "Gic for raising interrupt ")
	int_num = Param.UInt32("Interrupt number that connects to GIC")
	imgFile = Param.String (" flash image ")
	buffPort = MasterPort("")
	dmaBufferSize = Param.UInt64(16777216 , "buffer size for moving data between host memory and SSD internal memory")
	numChannel = Param.UInt32(2 , "Number of NAND Channel") 
	numPackages = Param.Int8(2 , "Number of NAND Channel")
	numDies = Param.Int16(12 , "Number of NAND Channel")
	numPlanes  = Param.UInt32(0 , "Number of NAND Channel") 
	numBlocks  = Param.UInt64(128 , "Number of NAND Channel")
	numPages   = Param.UInt64(128 , "Number of NAND Channel")
	media_type = Param.String ("slc" , "Media Type")
	nand_ltcy_rcmd_issue	= Param.Latency ( "5us" , "Issue read command to NAND die latency") 
	nand_ltcy_pcmd_issue	= Param.Latency ( "5us", "Issue program command to NAND die latency") 
	nand_ltcy_ecmd_issue	= Param.Latency ( "5us", "Issue erase command to NAND die latency")
	nand_ltcy_read_page		= Param.Latency ( "25us", "Read page latency")
	nand_ltcy_program_page  = Param.Latency ( "200us", "Program page latency")
	nand_ltcy_erase_block	= Param.Latency ( "2ms", "Erase block latency")
	nand_ltcy_read_msb_page = Param.Latency ( "50us" , "")
	nand_ltcy_read_lsb_page = Param.Latency ( "50us" , "")
	nand_ltcy_program_msb_page = Param.Latency ( "600us" , "")
	nand_ltcy_program_lsb_page = Param.Latency ( "900us" , "")
	nand_ltcy_read_csb_page = Param.Latency ( "75us", "")
	nand_ltcy_program_csb_page = Param.Latency ( "75us", "")
	nand_ltcy_read_chsb_page = Param.Latency ( "100us", "")
	nand_ltcy_read_clsb_page = Param.Latency ( "100us", "")
	nand_ltcy_program_chsb_page = Param.Latency ( "1800us", "")
	nand_ltcy_program_clsb_page = Param.Latency ( "2100us", "")
	chnBufferSize = Param.UInt64(16777216 , "Number of NAND Channel")
	DS = Param.Int8(9 , "Nand Page Size")
	index_num = Param.Int16(0,"Index")
	chn_0_index = Param.Int16(0,"chn 0 index")
	chn_1_index = Param.Int16(0, "chn 1 index")


class iSSDStats(DmaDevice):
	type = 'iSSDStats'
	cxx_header = "iSSDStats.hh"
	pio_addr = Param.Addr("")
	pio_size = Param.Addr("Size")
	dma = MasterPort("DMA port")
	pio_latency = Param.Latency("1ns", "Time between action and write/read ")
	gic = Param.BaseGic(Parent.any, "Gic for raising interrupt ")
	int_num = Param.UInt32("Interrupt number that connects to GIC")
	stats_updateTick_ltcy = Param.Latency("1us" ,"")

