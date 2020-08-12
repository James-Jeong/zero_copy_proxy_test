#include "local/jevtmgr_l.h"

///////////////////////////////////////////////////
// function for jevtmgr
///////////////////////////////////////////////////

jevtmgr_t *jevtmgr_create( size_t num)
{
	int rv;
	
	jevtmgr_t *evtmgr = ( jevtmgr_t*)malloc( num);
	if( evtmgr == NULL) {
		printf("Fail to allocate jevtmgr\n");
		return NULL;
	}

	rv = jevtmgr_init( evtmgr);
	if( rv < 0) {
		printf("Fail to initialize jevtmgr\n");
		jevtmgr_destroy( evtmgr);
		return NULL;
	}

	return evtmgr;
}

int jevtmgr_init( jevtmgr_t *evtmgr)
{
	int rv;

	evtmgr->events = dlist_ptr_create( MAX_LEN);
	if( evtmgr->events == NULL){
		return -1;	
	}

	evtmgr->is_alive = 0;
	evtmgr->timeout = 0;

	if( ( evtmgr->epoll_handle_fd = epoll_create( MAX_LEN)) < 0){
		printf("    | ! Proxy : Fail to create epoll handle fd\n");
		return FD_ERR;
	}

	return 1;
}
	
void jevtmgr_final( jevtmgr_t *evtmgr)
{
	int i;
	jevent_t *event;

	if( evtmgr->events){
		for( i = 0; i < evtmgr->events->length; i++) {
			event = ( jevent_t*)dlist_ptr_find_last_node_data( evtmgr->events);
			if( event) {
				dlist_ptr_del_node_by_data( ( void*)event, sizeof( event));
				jevent_destroy( event);
			}
		}
	}

	dlist_ptr_destroy( evtmgr->events);
	evtmgr->user_data = NULL;
}

void jevtmgr_destroy( jevtmgr_t *evtmgr)
{
	jevtmgr_final( evtmgr);
	free( evtmgr);
}

void jevtmgr_join( jevtmgr_t *evtmgr)
{
	
}

int jevtmgr_add_event( jevtmgr_t *evtmgr, jevent_t *event, short evtype, void *data, jevtmgr_handle_event_f callback)
{

}

int jevtmgr_remove_event( jevtmgr_t *evtmgr, jevent_t *event)
{

}

int jevtmgr_change_event( jevtmgr_t *evtmgr, jevent_t *event)
{

}

