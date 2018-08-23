/*
 * File: iSSDNandctrl.hh
 * Date: 2017. 05. 05.
 * Author: Thomas Haywood Dadzie (ekowhaywood@hanyang.ac.kr)
 * Copyright(c)2017 
 * Hanyang University, Seoul, Korea
 * Security And Privacy Laboratory. All right reserved
 */ 

#ifndef __DEV_ARM_iSSDNandCtrl_HH__
#define __DEV_ARM_iSSDNandCtrl_HH__

#define MAX_PAGES_PER_BLOCK 		256  // TODO , define in global limits
#define MAX_NUM_CHANNELS 			64

#include <queue>
#include <deque>

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

#include "params/iSSDNandCtrl.hh"
#include "../../../include/issd_types.h"
#include "../../../include/iSSDNandCtrlCmdReg.h"

//#include "params/iSSDHostInterface.hh"
//#include "../../../include/issd_types.h"
//#include "../../../include/nvmeDataStructures.h"
//#include "../../../include/host_ssd_interface.h"

#define  DPRINT_FILE stderr
//#define  DPRINTF2( class , fmt ,  var_args... )  fprintf ( DPRINT_FILE  , fmt , var_args  ) ; fflush(DPRINT_FILE) 
#define  DPRINTF2( class , fmt ,  var_args... )  

//#define  DPRINTF_LESS( class , fmt ,  var_args... )  fprintf ( DPRINT_FILE  , fmt , var_args  ) ; fflush(DPRINT_FILE) 
#define  DPRINTF_LESS( class , fmt ,  var_args... )  

  
class iSSDNandCtrl : public DmaDevice  
{
	
public:
	typedef iSSDNandCtrlParams Params;
	friend class PageDataPacket ;
	
	
	class PageDataPacket{
		
		
	private:
		uint8_t *data;
		uint8_t units ; 
		uint64_t size ;
		
		
	public:
		
		bool dataIsReady ;
		
		//		PageDataPacket ( uint8_t * srcPtr , uint64_t dataSize , uint8_t n  )
		//		{
		//			data = (uint8_t*)malloc(dataSize);
		//			units = n ;
		//			size = dataSize ;
		//		}
		
		uint64_t setData (const uint8_t * srcPtr ){
			memcpy (data , srcPtr , size);
			dataIsReady = true ;
			return size ;
		}
		
		PageDataPacket ( uint64_t datasize ){
			size = datasize ;
			data = (uint8_t*)malloc(size);
			units = size / 512  ;
			dataIsReady = false ;
		}
		
		PageDataPacket ( const uint8_t *srcPtr ,  uint64_t datasize ){
			
			//PageDataPacket (   unitCount ) ;
			size = datasize ;
			data = (uint8_t*)malloc(size);
			units =  size / 512 ;
			dataIsReady = false ;

			
			setData(srcPtr);
			
		}
		//		PageDataPacket ( uint8_t * srcPtr , uint64_t size ){
		//			PageDataPacket (srcPtr , size , (uint8_t)size >> 9 ); 
		//		}
		
		~PageDataPacket()
		{
			if(data)free (data);
		}
		void Destroy(){
			free(data) ; data = NULL ;
		}
		uint8_t * getPtr() { return data ;}
		uint8_t * getPtr(uint8_t index ){ return data + (index * 512); }
		uint64_t getSize() { return size ; }
		uint64_t getData(uint8_t * dst ) {
			
			// Ensure data is copied 
			if (dataIsReady ){
				memcpy (dst , data , size );
				return size ;
			}
			
			return 0 ;
		}
		
	};
	
	
	NAND_CTRL_CONFIG * config;
	   
		
	iSSDNandCtrl(const Params *p);
	  ~iSSDNandCtrl(){};
	
	/**  configurable parameters in python script **/
	
	/* Host controller information */
	const Addr pioAddr;
	const Addr pioSize;
	const Tick pioDelay; 
	
	/*  interrupt Number to cause door bell value change */
	const int intNum;
	/* Pointer to the GIC for causing an interrupt */
	BaseGic *gic;
	
	/* Address range functions */
	AddrRangeList getAddrRanges() const;

	/* Number of Nand controller Channels  , maximum of 128 allowes */
	uint32_t numChannel ;
	uint8_t numPackages; 
	uint8_t DS ;
	uint16_t numDies;
	uint32_t numPlanes; 
	uint32_t numBlocks ;
	uint32_t numPages  ;
	uint32_t imgPages ;
	uint32_t imgPagesMapped ; 
	
	std::string media_type ;
	
	Tick ltcy_rcmd_issue ;
	Tick ltcy_pcmd_issue ;
	Tick ltcy_ecmd_issue ; 
	Tick ltcy_read_page ;
	Tick ltcy_program_page ;
	Tick ltcy_erase_block ;
	
	Tick nand_ltcy_read_msb_page ;
	Tick nand_ltcy_read_lsb_page ;
	Tick nand_ltcy_program_msb_page; 
	Tick nand_ltcy_program_lsb_page ;
	Tick nand_ltcy_read_csb_page ;
	Tick nand_ltcy_program_csb_page;
	Tick nand_ltcy_read_chsb_page ;
	Tick nand_ltcy_read_clsb_page ;
	Tick nand_ltcy_program_chsb_page; 
	Tick nand_ltcy_program_clsb_page ;
	
	uint16_t NC_index ;
	uint32_t chnBufferSize; 
	
	/*  register access functions */
	Tick read(PacketPtr pkt);
	Tick write(PacketPtr pkt);
	
	nandCtrlReg Registers ;

	/*  DMA buffer */
	unsigned char * dmaBuffer;
	Addr dmaBufferSize;
	
	void shutdown();
	
	std::string imgFile;
	uint64_t imgSize;
	uint64_t imgPtr;
	void * loadImgProcess();
	//void INITCTRLComplete();
	//EventWrapper< iSSDNandCtrl , & iSSDNandCtrl::INITCTRLComplete > 
	//INITCTRLCompleteEvent;
	
	PageDataPacket * ImageManager_getData();// uint32_t page , NandPlane * plane , Channel * chn );
	uint64_t ImageManager_setData( );//PageDataPacket * src_pdpkt , uint32_t page ,	NandPlane * plane , Channel * chn );
	
	/* Stats */
	// Stats reporting
	Stats::Scalar cpu_reads ;
	Stats::Scalar cpu_writes ;
	
	void regStats() ;

	  	
	// ==============
	
	class NC_Channel;
	class NC_Command;
	class NC_Chip;
	class NC_Dma_Engine;
	class EventCompletionScheduler_1 ;
	
	class NC_Chip {
		
		public :
		iSSDNandCtrl * parent;

		iSSDNandCtrl::NC_Channel * parent_chn;
		
		int chip_index ;
		
		uint8_t cmd_1 ;
		uint8_t cmd_2;
		
		uint8_t address_bytes[5];
		int address_count;
		
		bool busy;
	
		unsigned int page_size ;
		unsigned char *nand_page_reg ;
	
		void clear_address(){
			address_bytes[0] = 0x00;
			address_bytes[1] = 0x00;
			address_bytes[2] = 0x00;
			address_bytes[3] = 0x00;
			address_bytes[4] = 0x00;
			address_count = 0;
		}
		
		typedef struct {
		
		uint32_t	row_block ;
		uint32_t	row_page ;
		uint32_t	column ;
			
		}nand_path_t ;
		
		
		int decode_address_cycles_5(nand_path_t * path ){
			
			address_cycle_t a ;
			a.cycle[0].byte = address_bytes[0];
			a.cycle[1].byte = address_bytes[1];
			a.cycle[2].byte = address_bytes[2];
			a.cycle[3].byte = address_bytes[3];
			
			path->row_page = a.shorts.short_1;
			path->row_block = a.shorts.short_2;

			return 1;
		}
		
		int decode_address_cycles_3( nand_path_t * path){
			
			address_cycle_t a ;
			a.cycle[0].byte = address_bytes[0];
			a.cycle[1].byte = address_bytes[1];
			a.cycle[2].byte = address_bytes[2];
			a.cycle[3].byte = address_bytes[3];
			
			path->row_block = a.shorts.short_2;
			
			return 1;
		}
		
		int decode_address_cycles_2(nand_path_t * path ){
			return 1;
		}
		
		Tick get_read_ltcy(uint32_t page_index );
		Tick get_program_ltcy(uint32_t page_index ) ;
		
		Tick do_read(){
		 
			busy= true;
			
			// decode address cycles 
			nand_path_t path ;
			decode_address_cycles_5( &path );
			
			// get data from image mgt 
			uint32_t planes_per_chn  = parent->numPackages * parent->numDies * parent->numPlanes ;
			uint32_t pages_per_plane = parent->numBlocks * parent->numPages ;
			uint32_t pages_per_chn   = planes_per_chn * pages_per_plane ;
			
			uint32_t page =	( parent_chn->chn_index * pages_per_chn ) +
							( chip_index * pages_per_plane) + 
							( path.row_block * parent->numPages )
							+ path.row_page ;
			
			//fprintf(stdout , "\n ** do read  NC:%u, chn:%u , chip :%u  blk:%u , page:%u ~~ page offset=%u , parent->imgPtr:%lx , nand_page_reg:%lx , page_size:%u" , 
			//	   parent->NC_index, parent_chn->chn_index,
			//	   chip_index , path.row_block , path.row_page ,
			//	   page , (uint64_t)parent->imgPtr , (uint64_t)nand_page_reg , 
			//		page_size  );
			//fflush(stdout);
		
			uint64_t src_ptr =  (parent->imgPtr + ( page * page_size ));
			
			memcpy ( nand_page_reg , (void *)src_ptr , page_size  );
			
			return get_read_ltcy(path.row_page); 
		}
		
		Tick do_program(){
		
			busy = true;
			
			// decode address cycles 
			nand_path_t path ;
			decode_address_cycles_5( &path );
			
			// send data to image mgt 
			uint32_t planes_per_chn = parent->numPackages * parent->numDies * parent->numPlanes ;
			uint32_t pages_per_plane = parent->numBlocks * parent->numPages ;
			uint32_t pages_per_chn = planes_per_chn * pages_per_plane ;
			
			uint32_t page = ( parent_chn->chn_index * pages_per_chn ) +
							( chip_index * pages_per_plane) + 
							( path.row_block * parent->numPages )
							+ path.row_page ;
			
			//fprintf(stdout, "\n ** do program  NC:%u, chn:%u , chip :%u  blk:%u , page:%u ~~ page offset=%u , parent->imgPtr:%#lx , nand_page_reg:%#lx , page_size:%u" , 
			//	   parent->NC_index, parent_chn->chn_index,
			//	   chip_index , path.row_block , path.row_page ,
			//	   page , (uint64_t)parent->imgPtr , (uint64_t)nand_page_reg , page_size  );
			//fflush(stdout);

			
			uint64_t dst_ptr =  (parent->imgPtr + ( page * page_size ));
			
			memcpy ((void*) dst_ptr  ,nand_page_reg , page_size  );
			
			return get_program_ltcy( path.row_page) ;
		}
	
		Tick do_erase(){
		
			busy = true;
			
			// decode address cycles 
			nand_path_t path ;
			decode_address_cycles_3( &path );
			
			uint32_t planes_per_chn = parent->numPackages * parent->numDies * parent->numPlanes ;
			uint32_t pages_per_plane = parent->numBlocks * parent->numPages ;
			uint32_t pages_per_chn = planes_per_chn * pages_per_plane ;
			
			uint32_t page = ( parent_chn->chn_index * pages_per_chn ) +
			( chip_index * pages_per_plane) + 
			( path.row_block * parent->numPages );

			uint64_t dst_ptr =  (parent->imgPtr + ( page * page_size ));

			fprintf(stdout, "\n ** do erase  NC:%u, chn:%u , chip :%u  blk:%u , page:0 ~~ page offset=%u , \
					parent->imgPtr:%#lx , dst_ptr:%#lx size:(%u * %u)" , 
					parent->NC_index, parent_chn->chn_index,
					chip_index , path.row_block ,
					page , (uint64_t)parent->imgPtr , dst_ptr,
					page_size , parent->numPages );
			fflush(stdout);
			
			// set pages to 0s  
			memset ((void*) dst_ptr  ,0 , page_size * parent->numPages );
		
			return parent->ltcy_erase_block ;
		}

		
		NC_Command * pending_write_cmd;
		
		NC_Chip(const iSSDNandCtrlParams * p , iSSDNandCtrl * parent_ ,
				NC_Channel* parent_chn_ , int chip_index_)
		{
			busy = false ;
			parent = parent_ ;
			parent_chn = parent_chn_;
			chip_index = chip_index_ ;
			page_size = 1 << parent->DS ;
			nand_page_reg = (unsigned char *)malloc(page_size);
			pending_write_cmd = (NC_Command*)0;
			
			//fprintf(stdout , "\n\t new chip object , chn:%u chip:%u , page_size:%u   " ,
			//		parent_chn->chn_index ,
			//		chip_index , page_size );
			//fflush(stdout);
		}
		
		
		Tick write_command_cycle( uint8_t nand_cmd){
			
			switch (nand_cmd) {
					case 0x80:
					case 0x00:
					case 0x60:
					clear_address();
					cmd_1 = nand_cmd;
					break;
					
					case 0x30:
					cmd_2 = nand_cmd;
					return do_read();
					break;
					
					case 0x10:
					cmd_2 = nand_cmd;
					return do_program();
					break;
					
					case 0xD0:
					cmd_2 = nand_cmd;
					return do_erase();
					break ;
					
				default:
					break;
			}
		
			return 0;
		}
		
		Tick write_address_cycle( uint8_t address_byte ){
			
			address_bytes[address_count] = address_byte;
			
			address_count ++;

			return 0;
		}
		
		Tick read_status( uint32_t * status_value ){
			//TODO 
			*status_value = 0;
			return 0 ;
		}
		
		bool is_busy(){ return busy ; }
		
		void * get_page_reg_ptr(){
			return  (void *)nand_page_reg;
		}
		
		void  operation_complete() 
		{
			busy = false ;
		}		
	};
	
	
	class NC_Command {
	public:
		command_t cmd ;
		NC_Chip * chip_enabled;
		address_cycle_t address_cycle;
		uint32_t ram_ptr ;
		uint32_t op ;
		bool data_phase_required ;
		
		NC_Command(){
			data_phase_required = false ;
		}
	};


	// simple fifo queue , fast but not thread safe
	class Simple_FiFo{
	
	public:
		typedef struct Entry{
			unsigned int 	data_type;
			void *		data ;
		}Entry ;
		
	private:
		unsigned int tail;
		unsigned int head;
		unsigned int en_size;
		unsigned int items;
		bool isEmpty;
		bool isFull;
		uint32_t default_data_type;
		Entry *entries ;

	public:
		
		Simple_FiFo( unsigned int  default_data_type_ ){
		
			tail = 0;
			head = 0;
			items = 0;
			en_size = 1024;
			isEmpty = true;
			isFull = false;
			default_data_type = default_data_type_ ;
			
			entries = (Entry*)malloc(sizeof(Entry) * en_size);
		}
		 		 
		bool empty(){ return  isEmpty ;}
		bool full(){ return isFull ;}
		uint32_t size(){ return items ;}
		
		int32_t push(void * data , unsigned int data_type ){
			
			if(isFull)return 0;
			
			entries[tail].data = data;
			entries[tail].data_type = data_type ;
			
			// advance tail 
			tail = tail + 1;
			tail = tail % en_size ;
			
			items = items + 1;
			
			if(head == (tail + 1) ){
				isFull = true ;
			}
			
			isEmpty = false ;
			return 1;
		}
		
		int32_t push(void * data){
			return push(data, default_data_type);
		}
		
	
		int32_t front (Entry *en ){
			
			if (isEmpty)return 0;
			
			en->data_type	= entries[head].data_type;
			en->data	= entries[head].data;
			
			return 1;	
		}
		
		int32_t pop_front(){
			
			if (isEmpty)return 0;
			
			// advance head 
			head ++;
			head = head % en_size ;
			items--;
			
			if (head == tail ){
				isEmpty = true ;
			}
			
			isFull = false;
			return 1;
		}

		int32_t pop(Entry *en){
			if (front(en)){
				pop_front();
				return 1;
			}
			return 0;
		}
	};
	
	
	class NC_Channel {
	private:		
		iSSDNandCtrl * parent;
		
	public:
		int chn_index ;
		iSSDNandCtrl::NC_Chip *chips[128] ;
		int chip_count ;
		int DS; 
		iSSDNandCtrl::NC_Command * new_cmd;
		
		iSSDNandCtrl::Simple_FiFo *cmd_waiting_room ;
		
		uint8_t active_nand_ops;
		bool wait_dma_completion ;
		bool read_dma;
		bool write_dma;
		uint32_t dma_ram_ptr;
		iSSDNandCtrl::NC_Chip *dma_chip;
		
		void add_cmd_phase( NC_Command * cmd){
			
			//fprintf ( stdout , "\n channel %u  add cmd phase cmd " , chn_index);
			//fflush(stdout);
			
			cmd->op = (0x0000FFFF & cmd->cmd.data) | 0x80000000 ;
			
			if (cmd->op == PROGRAM_NAND){
				cmd->data_phase_required = true;
				//fprintf ( stdout , ", data phase required  " );
				//fflush(stdout);
			}
			
			cmd_waiting_room->push((void *)cmd);
			
			//fprintf ( stdout , "  ,  w room count:%u" , cmd_waiting_room->size());
			//fflush(stdout);
		}
		
		void add_data_phase(NC_Command * cmd){
			//fprintf(stdout , " data phase"); fflush(stdout);
			cmd->op = cmd->cmd.data ;
			cmd_waiting_room->push((void*)cmd);
		}
		
		uint32_t cmd_waiting_room_size(){
			 return cmd_waiting_room->size();
		}
	
		NC_Channel(const iSSDNandCtrlParams * p , int num_chips , iSSDNandCtrl * parent_ , int index){
		
			chn_index = index ;
			parent = parent_ ;
			active_nand_ops = 0;
			wait_dma_completion = false ;
			read_dma = false;
			write_dma = false;
			DS =  p->DS ;
			cmd_waiting_room = new iSSDNandCtrl::Simple_FiFo(1);
			new_cmd = new iSSDNandCtrl::NC_Command();
			chip_count = num_chips;
			
			fprintf(stdout , "\nchn object: nc:%d, index:%d , num of chips:%d\n" ,parent_->NC_index , index, num_chips );
			fflush(stdout);
			
			for ( int i = 0 ; i < num_chips ; i++){
				NC_Chip *chip = new NC_Chip(p , parent_ , this , i ); 
				chips [i] = chip ;
			}
		}
	
	};
	
	

	class NC_Dma_Engine{
	public:
		
		int schedule_count ;
		//std::map<int , NC_Channel *> channels;
		iSSDNandCtrl * parent ;
		bool toggle ;
		iSSDNandCtrl::NC_Channel  * current_chn;
		
		void run_dma(){
			//fprintf(stdout , "\n run dma "); fflush(stdout);
			
			if (toggle ) {	// pick chn 0 first 
				
			    if ( parent->channel_0->wait_dma_completion ){
					current_chn = parent->channel_0;
				}	
				else if (parent->channel_1->wait_dma_completion){
					current_chn = parent->channel_1;
				}
				
				toggle = ! toggle;
			}
			else {	// pick chn 1 first 
				
				if ( parent->channel_1->wait_dma_completion ){
					current_chn = parent->channel_1;
				}	
				else if (parent->channel_0->wait_dma_completion){
					current_chn = parent->channel_0;
				}
							
				toggle = !toggle;
			}
			
			
			if (!current_chn) { 
				//fprintf(stdout, "  , no dma required , will exit");fflush(stdout);
				return ;
			}	
			
			unsigned char * ptr = (unsigned char *)current_chn->dma_chip->get_page_reg_ptr();
			
			
			if (current_chn->read_dma){
				
				//fprintf(stdout, "  , channel:%u , chip:%u : READ_DMA , ram:%#x" , 
				//		current_chn->chn_index, 
				//		current_chn->dma_chip->chip_index,
				//		current_chn->dma_ram_ptr);
				//fflush(stdout);
				
				EventCompletionScheduler_1 *callBack = new EventCompletionScheduler_1(parent);
				parent->dmaRead((Addr) current_chn->dma_ram_ptr
						, (1 << current_chn->DS)  , callBack , ptr );
		
			}
			else if (current_chn->write_dma){
				
				//fprintf(stdout, "  , channel:%u , chip:%u : WRITE_DMA , ram:%#x" , 
				//		current_chn->chn_index, 
				//		current_chn->dma_chip->chip_index,
				//		current_chn->dma_ram_ptr);
				//fflush(stdout);
				
				EventCompletionScheduler_1 *callBack = new EventCompletionScheduler_1(parent);
				parent->dmaWrite((Addr) current_chn->dma_ram_ptr
						, (1 << current_chn->DS)  , callBack , ptr  );

			}
			else{
				//fprintf(stdout, "  , channel:%u , chip:%u : error , not read nor write " , 
				//		current_chn->chn_index, 
				//		current_chn->dma_chip->chip_index);
				//fflush(stdout);
			}
		}
		
			
		NC_Dma_Engine( iSSDNandCtrl * parent_   )
		{
			schedule_count = 0;
			current_chn = (NC_Channel*)0;
			parent = parent_ ;
		}
		
	};
	
	
	iSSDNandCtrl::NC_Channel *channel_0;
	iSSDNandCtrl::NC_Channel *channel_1;
	iSSDNandCtrl::NC_Dma_Engine *dma_engine;
	
	void process_dma() {
		if (dma_engine->current_chn) return ;
		dma_engine->run_dma();
	}
		
	void dma_complete_call_back (){
		
		NC_Channel * channel = dma_engine->current_chn;
		channel->active_nand_ops--;
		
		if (channel->read_dma){
			
			//fprintf(stdout , "\n read dma cmplt call back, channnel:%u , chip:%u , active_nand_ops:%u , @ %lu" ,
			//		channel->chn_index , channel->dma_chip->chip_index ,
			//		(unsigned int)channel->active_nand_ops , curTick()); 
			//fflush(stdout);
			
			// write the program second command cycle 
			if ( channel->dma_chip->pending_write_cmd ){
				
				NC_Command* cmd = channel->dma_chip->pending_write_cmd;
				channel->dma_chip->pending_write_cmd= (NC_Command* )0 ;
				Tick t1 = channel->dma_chip->write_command_cycle(cmd->cmd.cmd_phase.nand_end_cmd);
				channel->read_dma = false;
				channel->active_nand_ops++;
				
				// schedule chip status change event
				EventCompletionScheduler_2 *callBack = new EventCompletionScheduler_2(this , channel , channel->dma_chip );
				schedule(callBack , curTick() + t1 );
				
			}
		}
		else if ( channel->write_dma){
			
			//fprintf(stdout , "\n write dma cmplt call back,  channnel:%u , chip:%u , active_nand_ops:%u , @ %lu" ,
			//		channel->chn_index , 
			//		channel->dma_chip->chip_index ,
			//		(unsigned int)channel->active_nand_ops , curTick() ); 
			//fflush(stdout);
			
			channel->write_dma = false;
		}
		
		dma_engine->current_chn = (NC_Channel*)0;
		
		channel->dma_chip = (NC_Chip * )0; 
		channel->dma_ram_ptr = 0;
		channel->wait_dma_completion = false;
		
		dma_engine->run_dma();
		process_waiting_room_cmd(channel);
	}
	

	void process_waiting_room_cmd(NC_Channel *chn){
		
		iSSDNandCtrl::Simple_FiFo::Entry en;
		iSSDNandCtrl::NC_Command * cmd ;
	
		//fprintf(stdout , "\n @ %lu : process cmd: channel:%u : w room count:%u , wait_dma_completion:%d " , curTick(), chn->chn_index ,
		//		chn->cmd_waiting_room->size() ,  
		//		chn->wait_dma_completion );
		//fflush(stdout);
	
		if (chn->wait_dma_completion)return ;

		if(chn->cmd_waiting_room->empty()) return ;
		
		chn->cmd_waiting_room->front(&en);
		cmd = (NC_Command*)en.data ;
		
		//fprintf(stdout , " , chip busy:%d " , cmd->chip_enabled->is_busy() );
		//fflush(stdout );
		if (cmd->chip_enabled->is_busy()) return ;
		
		if ( cmd->op == READ_NAND){
			
			Tick t1 , t2 , t3 ;
			t1 = t2 = t3 = 0;
			
			//fprintf(stdout , "\t : READ_NAND " ); fflush(stdout );
		
			t1 = cmd->chip_enabled->write_command_cycle(cmd->cmd.cmd_phase.nand_start_cmd); 
			
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[0].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[1].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[2].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[3].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[4].byte );
			
			t3 = cmd->chip_enabled->write_command_cycle(cmd->cmd.cmd_phase.nand_end_cmd); 
			
			chn->active_nand_ops ++ ;
			
			// schedule chip status change event
			EventCompletionScheduler_2 *callBack = new EventCompletionScheduler_2(this ,chn, cmd->chip_enabled );
			schedule(callBack , curTick() + (t1 + t2 + t3) );
			
		}
		else if (cmd->op == PROGRAM_NAND){
			
			//fprintf(stdout , "\t : PROGRAM_NAND"  ); fflush(stdout);
			
			cmd->chip_enabled->write_command_cycle(cmd->cmd.cmd_phase.nand_start_cmd); 
			
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[0].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[1].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[2].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[3].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[4].byte );

			// program second command cycle will be issued after dma operation
			cmd->chip_enabled->pending_write_cmd = cmd ;
			
		}
		else if (cmd->op == ERASE_NAND){
			
			Tick t1 , t2 , t3 ;
			t1 = t2 = t3 = 0;
			
			//fprintf(stdout , "\t : ERASE_NAND "   ); fflush(stdout);
			
			t1 = cmd->chip_enabled->write_command_cycle(cmd->cmd.cmd_phase.nand_start_cmd); 
			
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[0].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[1].byte );
			t2 += cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[2].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[3].byte );
			cmd->chip_enabled->write_address_cycle(cmd->address_cycle.cycle[4].byte );
			
			t3 = cmd->chip_enabled->write_command_cycle(cmd->cmd.cmd_phase.nand_end_cmd); 
			
			chn->active_nand_ops++;
			
			// schedule chip status change event
			EventCompletionScheduler_2 *callBack = new EventCompletionScheduler_2(this , chn, cmd->chip_enabled );
			schedule(callBack , curTick() + (t1 + t2 + t3) );
			
		}
		else if (cmd->op == READ_DMA){
			//fprintf(stdout , "\t : READ_DMA " ); fflush(stdout);
			
			chn->active_nand_ops++;
			chn->read_dma = true;
			chn->dma_ram_ptr = cmd->ram_ptr;
			chn->dma_chip = cmd->chip_enabled;
			chn->wait_dma_completion = true;
			process_dma() ;
			
		}
		else if (cmd->op == WRITE_DMA){
			//fprintf(stdout , "\t : WRITE_DMA" ); fflush(stdout);
			
			chn->active_nand_ops++;
			chn->write_dma = true;
			chn->dma_ram_ptr = cmd->ram_ptr;
			chn->dma_chip = cmd->chip_enabled;
			chn->wait_dma_completion = true;
			process_dma() ;
		}
		else {
			
			fprintf(stdout , " : command not supported"); fflush(stdout);
		}
		
		chn->cmd_waiting_room->pop_front();
		
		process_waiting_room_cmd(chn );
	}
	
	void operation_complete_call_back ( NC_Channel * channel , NC_Chip * chip  ){
		
		channel->active_nand_ops--;
		
		//fprintf (stdout , "\n op complete , active_nand_ops:%u @ %lu" ,channel->active_nand_ops , curTick());
		//fflush(stdout);
		
		chip->operation_complete();
		
		process_waiting_room_cmd(channel);
		
	}

	
	class EventCompletionScheduler_1 : public Event
	{
	public:
		iSSDNandCtrl *owner ;
	
		EventCompletionScheduler_1(iSSDNandCtrl *owner ) :
		Event(Default_Pri, AutoDelete), owner(owner)
		{ }
		
		/** Process the event. Just call into the  owner. */
		void process() override {
			owner->dma_complete_call_back( );
		}
	};
	
	class EventCompletionScheduler_2 : public Event
	{
	public:
		iSSDNandCtrl *owner ;
		NC_Channel * channel ;
		NC_Chip * chip ;
	
		EventCompletionScheduler_2(iSSDNandCtrl *owner , NC_Channel* chn, NC_Chip *chip_) :
		Event(Default_Pri, AutoDelete), owner(owner),channel(chn),  chip(chip_)
		{ }
		
		/** Process the event. Just call into the  owner. */
		void process() override {
			owner->operation_complete_call_back(channel, chip );
		}
	};
		
};

  


#endif




















