 
#include <hw_drivers.h>
#include "issd_kernel.h"

#define R_SIZE (INTERNAL_IO_REQ_QUEUE*2) 
r_req_t		r_req[R_SIZE];
DEFINE_CIRCULAR_QUEUE(free_r_req_objs , R_SIZE )  

#define W_SIZE (INTERNAL_IO_REQ_QUEUE*2) 
w_req_t		w_req[W_SIZE];
DEFINE_CIRCULAR_QUEUE(free_w_req_objs , W_SIZE )

#define O_SIZE 128
char *	other_req_alloc[O_SIZE][MAX_REQUEST_OBJ_MEM_SIZE]; 
DEFINE_CIRCULAR_QUEUE (free_req_objs, O_SIZE)



void init_req_obj(){
	
	u32 i ;
	req_common_t * req ;
	
	// READ_IO_REQUEST
	i = 0 ;
	while ((req = enQueue(&free_r_req_objs, & r_req[i++])))
	{
		req->type = READ_IO_REQUEST;
		req->index = i;
		req->req_stage = REQ_STAGE_FREE ;
	}

	
	// WRITE_IO_REQUEST
	i = 0 ;
	while ((req  = enQueue(&free_w_req_objs, & w_req[i++])))
	{
		req->type = WRITE_IO_REQUEST;
		req->index = i;
		req->req_stage = REQ_STAGE_FREE ;
	}
	

	// OTHERS
	if(sizeof(f_req_t) > MAX_REQUEST_OBJ_MEM_SIZE ){
		VPRINTF_ERROR (" Error in [%s]: sizeof(f_req_t) > MAX_REQUEST_OBJ_MEM_SIZE" , __FUNCTION__);
		shutDownSystem();		
	}
	if(sizeof(isp_req_t) > MAX_REQUEST_OBJ_MEM_SIZE ){
		VPRINTF_ERROR (" Error in [%s]: sizeof(isp_req_t) > MAX_REQUEST_OBJ_MEM_SIZE", __FUNCTION__);
		shutDownSystem();		
	}
	
	i = 0 ;
	while ((req = enQueue(&free_req_objs, &(other_req_alloc[i++][0]))))
	{
		req->index = i;
		req->req_stage = REQ_STAGE_FREE ;
	}
	
}

void * sys_malloc_req_obj(uint32_t req_type){
	
	req_common_t *req ;
	
	switch (req_type) {
			
		case READ_IO_REQUEST:
			if ((req = deQueue(&free_r_req_objs)) == NULL){
				VPRINTF_ERROR (" **** %s Object Allocation Error , Free Queue Empty **** ", request_type_string[req_type] ) ;
			} 
			break;
			
		case WRITE_IO_REQUEST:
			if ((req = deQueue(&free_w_req_objs)) == NULL){
				VPRINTF_ERROR (" **** %s Object Allocation Error , Free Queue Empty **** ", request_type_string[req_type] ) ;
			} 
			break;
			
		case ADMIN_REQUEST:
		case FLUSH_IO_REQUEST:
		case ISP_IO_REQUEST:
		case ISP_READ_PAGES_REQUEST:
		case ISP_PROGRAM_PAGES_REQUEST:
		case ISP_HOST_DMA_REQUEST:
		case TRIM_IO_REQUEST:
			if ((req = deQueue(&free_req_objs)) == NULL){
				VPRINTF_ERROR (" **** %s Object Allocation Error , Free Queue Empty **** ", request_type_string[req_type] ) ;
			}
			req->type = req_type ;			
			break;
	
		default:
			VPRINTF_ERROR (" **** Invalid Request Type ( requested type:%u) **** ", req_type ) ;
			return 0;
			break;
	}

	req->req_stage = REQ_STAGE_INUSE ;
	return req;
}


io_req_common_t * sys_find_io_request( u32 sqid , u32 cid ){


	for (u32 i = 0 ; i < R_SIZE ; i++){
		if ( r_req[i].sqid == sqid && r_req[i].scmd.cid == cid ){
			return (io_req_common_t*) &r_req[i] ;
		}
	}

	for (u32 i = 0 ; i < W_SIZE ; i++){
		if (w_req[i].sqid == sqid && w_req[i].scmd.cid == cid ){
			return (io_req_common_t*) &w_req[i] ;
		}
	}


	for (u32 i = 0 ; i < O_SIZE ; i++){
		
		req_common_t* other_req = (req_common_t*)&(other_req_alloc[i][0]) ;
		io_req_common_t* io_req ;

		switch (other_req->type){
			case FLUSH_IO_REQUEST:
			case ISP_IO_REQUEST:
			case TRIM_IO_REQUEST:
				io_req = (io_req_common_t*)other_req ;
				if (io_req->sqid == sqid && io_req->scmd.cid == cid ){
					return io_req ;
				}
			break;

			default:
				break;
		}
	}

	return 0 ;
}


void * sys_get_active_req_objects(){

	VPRINTF(" ***  PRINTING ACTIVE REQ Objects  ***") ;

	VPRINTF ("  ");
	for (u32 i = 0 ; i < R_SIZE ; i++){
		
		io_req_common_t* req = (io_req_common_t*) & r_req[i] ;

		if ( req->req_stage != REQ_STAGE_FREE){
			
			VPRINTF ("\t %u : %u, [%u]%s, %s , sqid:%u, cid:%u " , 
				i , req->index ,
				req->type,request_type_string[req->type],
				request_stage_string[req->req_stage],
				req->sqid,
				req->scmd.cid );
		}
	}


	VPRINTF ("  ");

	for (u32 i = 0 ; i < W_SIZE ; i++){
		
		io_req_common_t* req = (io_req_common_t*) & w_req[i] ;

		if (req->req_stage != REQ_STAGE_FREE ){
			
			VPRINTF ("\t %u : %u, [%u]%s, %s , sqid:%u, cid:%u " , 
				i , req->index ,
				req->type,request_type_string[req->type],
				request_stage_string[req->req_stage],
				req->sqid,
				req->scmd.cid );
		}
	}

	VPRINTF ("  ");

	for (u32 i = 0 ; i < O_SIZE ; i++){
		
		req_common_t* req_obj = (req_common_t*)&(other_req_alloc[i][0]) ;
		
		if (req_obj->req_stage != REQ_STAGE_FREE ){

			if ((req_obj->type == FLUSH_IO_REQUEST )|| 
				(req_obj->type == ISP_IO_REQUEST) || 
				(req_obj->type == TRIM_IO_REQUEST) )
			{
				
				io_req_common_t* req = (io_req_common_t*) req_obj ;

				VPRINTF ("\t %u : %u, [%u]%s, %s , sqid:%u, cid:%u " , 
					i , req->index ,
					req->type,request_type_string[req->type],
					request_stage_string[req->req_stage],
					req->sqid,
					req->scmd.cid);

			}else{

				VPRINTF ("\t %u : %u, [%u]%s, %s " , 
					i , req_obj->index ,
					req_obj->type,request_type_string[req_obj->type],
					request_stage_string[req_obj->req_stage]);
			}
		
		}

	}


	return (void*)0;
}


void sys_free_req_obj(void * req_obj){
	
	if (! req_obj){
		VPRINTF_ERROR ( " *** Free ReqObj Error : Null Pointer *** ");
		return ;
	} 

	req_common_t* req = (req_common_t*)req_obj ;
	queue_t * queue ;

	if (req->req_stage != REQ_COMPLETED ){
		VPRINTF_ERROR (" **** Free ReqObj Error : Request not completed Req:{ %u, %s, stage:%s }", 
		req->index ,
		request_type_string[req->type],
		request_stage_string[req->req_stage]);
	}


	switch (req->type) {
			
		case READ_IO_REQUEST:
			queue = &free_r_req_objs ;
			break;
		case WRITE_IO_REQUEST:
			queue = &free_w_req_objs ;
			break;
		case ADMIN_REQUEST:		
		case FLUSH_IO_REQUEST:
		case ISP_IO_REQUEST:
		case TRIM_IO_REQUEST:
		case ISP_READ_PAGES_REQUEST:
		case ISP_PROGRAM_PAGES_REQUEST:
		case ISP_HOST_DMA_REQUEST:
			queue = &free_req_objs ;		
			break ;
		default:
			VPRINTF_ERROR (" *** Free Req Obj Error , Unknown req_type:%u ***  " , req->type );
			return ;
			break;
	}

	enQueue(queue, req );
	req->req_stage = REQ_STAGE_FREE ;
	
}
 


  




