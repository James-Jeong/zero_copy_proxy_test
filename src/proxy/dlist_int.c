#include "dlist_int.h"

dlist_int_t* dlist_int_create(){
	dlist_int_t *list = ( dlist_int_t*)malloc( sizeof( dlist_int_t));
	if( list == NULL){
		printf("		| ! List : Fail to create list object\n");
		return NULL;
	}

	list->head->data = 0;
	list->head->prev = NULL;
	list->head->next = list->tail;

	list->tail->data = 0;
	list->tail->prev = list->head;
	list->tail->next = NULL;

	list->length = 0;

	return list;
}

void dlist_int_init( dlist_int_t *list){
	memset( list, '\0', sizeof( dlist_int_t));
	memset( list->head, '\0', sizeof( node_int_t));
	memset( list->tail, '\0', sizeof( node_int_t));

	list->head->prev = NULL;
	list->head->next = list->tail;

	list->tail->prev = list->head;
	list->tail->next = NULL;

	list->length = 0;
}

void dlist_int_destroy( dlist_int_t *list){
	if( list){
		node_int_t *index_node = list->head;
		if( index_node != NULL){
			if( index_node->next != list->tail){
				while( index_node->next != NULL){
					index_node = index_node->next;
					dlist_int_del_node_by_node( list, index_node);
				}
			}
		}
//		free( list);
	}
}

int dlist_int_add_node( dlist_int_t *list, int data){
	node_int_t *new_node = node_int_create( data);
	if( new_node == NULL){
		printf("		| ! List : Fail to create node object in dlist_int_add_node\n");
		return OBJECT_ERR; 
	}

	if( list->head->next == list->tail){
		list->head->next = new_node;
		new_node->next = list->tail;
		new_node->prev = list->head;
		list->tail->prev = new_node;
	}
	else if( ( list->head->next != list->tail) && ( list->tail->prev != NULL)){
		node_int_t *old_node = list->tail->prev;

		old_node->next = new_node;
		new_node->prev = old_node;

		list->tail->prev = new_node;
		new_node->next = list->tail;
	}
	else{
		printf("		| ! List : unknown add node case in dlist_int_add_node\n");
		return UNKNOWN;
	}

	list->length++;

	return NORMAL;
}

int dlist_int_del_node_by_node( dlist_int_t *list, node_int_t *target){
	node_int_t *head = list->head;
	node_int_t *tail = list->tail;

	if( list->head->next == NULL){
		printf("		| ! List : Not found node in list\n");
		list->length = 0;
		return NOT_EXIST;
	}

	node_int_t *index_node = list->head->next;

	while( index_node != NULL){
		if( index_node == target){
			index_node->prev->next = index_node->next;
			index_node->next->prev = index_node->prev;
			node_int_destroy( index_node);
			break;
		}

		index_node = index_node->next;
	}

	list->length--;

	return NORMAL;
}

int dlist_int_del_node_by_data( dlist_int_t *list, int data){
	node_int_t *head = list->head;
	node_int_t *tail = list->tail;

	if( list->head->next == NULL){
		printf("		| ! List : Not found node in list\n");
		list->length = 0;
		return NOT_EXIST;
	}

	node_int_t *index_node = list->head->next;

	while( index_node != NULL){
		if( index_node->data == data){
			index_node->prev->next = index_node->next;
			index_node->next->prev = index_node->prev;
/*			if( index_node->prev == head){
				head->next = index_node->next;
				index_node->next->prev = head;
			}
			else if( index_node->next == tail){
				tail->prev = index_node->prev;
				index_node->prev->next = tail;
			}
			else{
				index_node->prev->next = index_node->next;
				index_node->next->prev = index_node->prev;
			}
*/
			node_int_destroy( index_node);
			break;
		}

		index_node = index_node->next;
	}

	list->length--;

	return NORMAL;
}

node_int_t* dlist_int_find_node_by_node( dlist_int_t *list, node_int_t *target){
	node_int_t *head = list->head;
	node_int_t *tail = list->tail;

	if( list->head->next == NULL){
		printf("		| ! List : Not found node in list\n");
		list->length = 0;
		return NULL;
	}

	node_int_t *index_node = list->head->next;

	while( index_node != NULL){
		if( index_node == target){
			return index_node;
		}

		index_node = index_node->next;
	}

	return NULL;
}

int dlist_int_find_node_data_by_node( dlist_int_t *list, node_int_t *target){
	node_int_t *head = list->head;
	node_int_t *tail = list->tail;

	if( list->head->next == NULL){
		printf("		| ! List : Not found node in list\n");
		list->length = 0;
		return NOT_EXIST;
	}

	node_int_t *index_node = list->head->next;

	while( index_node != NULL){
		if( index_node == target){
			return index_node->data;
		}

		index_node = index_node->next;
	}

	return NOT_EXIST;
}

node_int_t* dlist_int_find_node_by_data( dlist_int_t *list, int data){
	node_int_t *head = list->head;
	node_int_t *tail = list->tail;

	if( list->head->next == NULL){
		printf("		| ! List : Not found node in list\n");
		list->length = 0;
		return NULL;
	}

	node_int_t *index_node = list->head->next;

	while( index_node != NULL){
		if( index_node->data == data){
			return index_node;
		}

		index_node = index_node->next;
	}

	return NULL;
}

int dlist_int_find_first_node_data( dlist_int_t *list){
	return ( list->head->next != NULL)? list->head->next->data : list->head->data;
}

int dlist_int_find_last_node_data( dlist_int_t *list){
	return ( list->tail->prev != NULL)? list->tail->prev->data : list->tail->data;
}


// --------------------------------

node_int_t* node_int_create( int data){
	node_int_t *node = ( node_int_t*)malloc( sizeof( node_int_t));
	if( node == NULL){
		return NULL;
	}

	memset( node, '\0', sizeof( node));

	node->prev = NULL;
	node->next = NULL;
	node->data = data;
	return node;
}

void node_int_destroy( node_int_t *node){
	if( node){
		node->prev = NULL;
		node->next = NULL;
		free( node);
	}
}

