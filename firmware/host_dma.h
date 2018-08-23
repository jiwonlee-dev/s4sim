 
 
#ifndef HOST_DMA_HEADER
#define HOST_DMA_HEADER
 
#define READ_IO_DMA_OPERATION 0x60
#define WRITE_IO_DMA_OPERATION 0x70

#define setReg(field , value)	regData = ((reg *) &( field )) ; *regData = (reg)value ;

#define setReg64(field , value)						\
		regData = ( &( field .Dwords.lbits  )); 	\
		*regData = value .Dwords.lbits  ; 			\
		regData = ( &( field .Dwords.hbits  )) ; 	\
		*regData = value .Dwords.hbits  ;

#define getReg(field , dst )	regData = ((reg *) &( field )) ; dst = *regData  ;

#define getReg64(field, dst)							\
		regData = ((reg *) &( field  .Dwords.lbits )) ; \
		dst  .Dwords.lbits  = *regData  ; 				\
		regData = ((reg *) &( field  .Dwords.hbits )) ; \
		dst .Dwords.hbits  = *regData  ;


typedef struct {
	
	uint32_t dma_type ;
	uint32_t status ;
	
	uint32_t data_size ;
	u8* 	 ssd_addr;
	uint64_t host_p_addr ;
	
	uint32_t sqid ;
	uint32_t sqe ;
	uint32_t num_of_entries ;
	
	uint32_t cqid;
	uint32_t cqe ;
	uint32_t vector ;
	uint32_t irq_enabled ;
	
	uint32_t op;
	dma_list_t *list;
	
	r_req_t *read_req;
	w_req_t *write_req;
	
	
} sync_dma_t ;
 
void sync_dma_cbk(void * obj_ptr , uint32_t obj_type);

#endif






