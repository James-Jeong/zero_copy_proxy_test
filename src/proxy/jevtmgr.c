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
	evtmgr->thead_cnt = 0;
	evtmgr->stop = false;
}
	
void jevtmgr_final( jevtmgr_t *evtmgr)
{

}

void jevtmgr_destroy( jevtmgr_t *evtmgr)
{

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

