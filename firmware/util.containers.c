 

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>



void* enQueue ( queue_t *queue  , void * item_ptr){
	// check if full 
	if (queue->head == ((queue->tail + 1) % queue->size) ) { return NULL ; } 
	// add to items 
	queue->objs[queue->tail] = (void*)item_ptr ;
	// adjust the tail pointer
	queue->tail = (queue->tail+ 1 ) % queue->size ;
	
	return item_ptr ;
}

void * deQueue (queue_t * queue ){
	// check if empty 
	if (queue->head == queue->tail){  return NULL ;  }
	// get the item 
	void * item_ptr = queue->objs[queue->head];
	// adjust the head pointer
	queue->head = (queue->head + 1 ) % queue->size ;
	
	return item_ptr;
}



 

/* 
 * FIFO Queue Implementation   
 */

/*
q_buffer_t * q_buffer;

queue_id init_queue(queue *q, queue_entry *en_ptr , uint32_t cpuID , 
					int32_t size , uint32_t default_data_type){
	
	q->tail = 0;
	q->head = 0;
	q->size = size ;
	q->isEmpty = true;
	q->isFull = false;
	q->entries = & q_buffer->ens[cpuID][q_buffer->ens_index[cpuID]] ;
	q_buffer->ens_index[cpuID] += size ;
	q->default_data_type = default_data_type;
	
	return (queue_id)q;
}

queue_id get_queue(uint32_t cpuID , int32_t size , uint32_t default_data_type){
	
	queue * q = & q_buffer->qs[cpuID][q_buffer->qs_index[cpuID]];
	q_buffer->qs_index[cpuID] ++ ;
	
	queue_entry * en_ptr =  & q_buffer->ens[cpuID][q_buffer->ens_index[cpuID]] ;
	q_buffer->ens_index[cpuID] += size ;
	
	init_queue(q , en_ptr , cpuID , size , default_data_type);
	
	return (queue_id)q ;
}

queue_id create_queue(int32_t size , uint32_t default_data_type){
	
	queue * q = (queue * ) get_uncacheable_ram(sizeof(queue));

	q->entries = (queue_entry * )get_uncacheable_ram(sizeof(queue_entry) * size);
	
	q->tail = 0;
	q->head = 0;
	q->size = size ;
	q->isEmpty = true;
	q->isFull = false;
	q->default_data_type = default_data_type;
	
	return (queue_id)q ;
}




int32_t push2(queue_id q_ , void * data_ptr , uint32_t data_type ){
	
	queue *q = (queue*)q_;
	
	if(q->isFull)return 0;
	
	q->entries[q->tail].data_ptr = data_ptr;
	q->entries[q->tail].data_type = data_type ;
	
	// advance tail 
	q->tail ++;
	q->tail = q->tail % q->size ;
	
	if(q->head == (q->tail + 1) ){
		q->isFull = true ;
	}
	
	q->isEmpty = false ;
	
	return 1;
}

int32_t push(queue_id q_ , void * data_ptr ){
	return push2(q_ , data_ptr , ((queue*)q_)->default_data_type);
}


int32_t pop(queue_id q_ , queue_entry *en ){
	
	queue *q = (queue*)q_;
	
	if (q->isEmpty)return 0;
	
	en->data_type  = q->entries[q->head].data_type;
	en->data_ptr = q->entries[q->head].data_ptr;
	
	// advance head 
	q->head ++;
	q->head = q->head % q->size ;
	
	if (q->head == q->tail ){
		q->isEmpty = true ;
	}
	
	q->isFull = false;
	
	return 1;

}

int32_t head_entry(queue_id q_ , queue_entry *en ){
	
	queue *q = (queue*)q_;
	
	if (q->isEmpty)return 0;
		
	en->data_type	= q->entries[q->head].data_type;
	en->data_ptr	= q->entries[q->head].data_ptr;
	
	return 1;	
}

int32_t pop_head( queue_id q_ ){

	queue *q = (queue*)q_;
	
	if (q->isEmpty)return 0;
	
	// advance head 
	q->head ++;
	q->head = q->head % q->size ;
	
	if (q->head == q->tail ){
		q->isEmpty = true ;
	}
	
	q->isFull = false;
	
	return 1;
}
*/

/* 
 * END : FIFO Queue Implementation   
 */

 
  

/* 
 * Linked List Implementation   
 */

/* 

llist_t * create_llist(int max_items){
	
	llist_t * new_ll  = (llist_t *)get_uncacheable_ram(sizeof(llist_t));
	node_t * nodes = (node_t *)get_uncacheable_ram(sizeof(node_t) * max_items);
	
	new_ll->free_nodes = create_queue(max_items , 0);
	
	for ( int i = 0 ; i < max_items ; i++){
		push(new_ll->free_nodes, &(nodes[i]));
	}
	
	new_ll->head = NULL;
	new_ll->current = NULL;
	
	return new_ll ;
}

//insert link at the first location
void llist_insertFirst(llist_t * list , int key,  void * data) {
	//create a link
	
	queue_entry en;
	
	if (! (pop(list->free_nodes,&en))){
		VPRINTF_ERROR (" fatal error : list 0x%x free_nodes empty ", (uint32_t)list );
		void shutDownSystem();
	}
		
	node_t  *link = en.data_ptr ;////= (node_t *) malloc(sizeof(node_t ));
	
	link->key = key;
	link->data = data;
	
	//point it to old first node
	link->next = list->head;
	
	//point first to new first node
	list->head = link;
}

//is list empty
bool llist_isEmpty(llist_t * list) {
	return list->head == NULL;
}

int llist_length(llist_t * list) {
	int length = 0;
	node_t  *current;
	
	for(current = list->head; current != NULL; current = current->next) {
		length++;
	}
	
	return length;
}

//find a link with given key
node_t * llist_find(llist_t * list ,int key) {
	
	//start from the first link
	node_t * current = list->head;
	
	//if list is empty
	if(list->head == NULL) {
		return NULL;
	}
	
	//navigate through list
	while(current->key != key) {
		
		//if it is last node
		if(current->next == NULL) {
			return NULL;
		} else {
			//go to next link
			current = current->next;
		}
	}      
	
	//if data found, return the current Link
	return current;
}

//delete first item
void llist_deleteFirst(llist_t * list ) {
	
	//save reference to first link
	node_t  *tempLink = list->head;
	
	//mark next to first link as first 
	list->head = list->head->next;
	
	push(list->free_nodes, tempLink);
	
	//return the deleted link
	//return tempLink;
}


//delete a link with given key
void llist_delete(llist_t * list ,int key) {
	
	//start from the first link
	node_t * current = list->head;
	node_t * previous = NULL;
	
	//if list is empty
	if(list->head == NULL) {
		return ;//NULL;
	}
	
	//navigate through list
	while(current->key != key) {
		
		//if it is last node
		if(current->next == NULL) {
			return ;//NULL;
		} else {
			//store reference to current link
			previous = current;
			//move to next link
			current = current->next;
		}
	}
	
	//found a match, update the link
	if(current == list->head) {
		//change first to point to next link
		list->head = list->head->next;
	} else {
		//bypass the current link
		previous->next = current->next;
	}    
	
	push(list->free_nodes, current);
	
	//return current;
}
 */


/* 
 * END : Linked List Implementation   
 */




