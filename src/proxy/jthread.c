#include "local/jthread_l.h"

///////////////////////////////////////////////////
// functions for jthread
///////////////////////////////////////////////////
jthread_t *jthread_create(){
	int rv;

	jthread_t *thread = ( jthread_t*)malloc( sizeof( jthread_t));
	if( thread == NULL){
		printf("Fail to allocate thread.\n");
		return NULL;
	}

	rv = jthread_init( thread);
	if( rv < 0){
		printf("Fail to initialize thread. (rv:%d)\n", rv);
		return NULL;
	}

	return thread;
}

int jthread_init( jthread_t *thread){
	id = -1;
	is_alive = 0;
	data = NULL;

	return 1;
}

void jthread_final( jthread_t *thread){
	data = NULL;
}

void jthread_destroy( jthread_t *thread){
	jthread_final( thread);
	free( thread);
}

int jthread_run( jthread_t *thread){

}

void jthread_stop( jthread_t *thread){

}

///////////////////////////////////////////////////
// functions for jthrmgr
///////////////////////////////////////////////////
jthrmgr_t *jthrmgr_create( size_t num){
	int rv;

	jthrmgr_t *thrmgr = ( jthrmgr_t*)malloc( num);
	if( thread == NULL){
		printf("Fail to allocate thread.\n");
		return NULL;
	}

	rv = jthrmgr_init( thrmgr);
	if( rv < 0){
		printf("Fail to initialize thread. (rv:%d)\n", rv);
		return NULL;
	}

	return thrmgr;
}

int jthrmgr_init( jthrmgr_t *thrmgr){
	thrmgr->threads = dlist_init( MAX_NTHREAD);
	if( thrmgr->threads == NULL){
		printf("Fail to initialize dlist for jthrmgr_t. (rv:%d)\n", rv);
		return -1;
	}

	thrmgr->nthread = 0;
	thrmgr->data = NULL;

	int i;
	jthread_t *thread;
	for( i = 0; i < MAX_NTHREAD; i++){
		thread = jthread_create();
		rv = pthread_create( &thread->id, NULL, jpool_worker, jpool);
		if( rv != 0){
			printf("Fail to create thread. (rv:%d)\n", rv);
			jthread_destroy( thread);
			return -1;
		}

		rv = dlist_ptr_add_node( thrmgr->thrmgr, thread);
		if( rv < 0){
			printf("Fail to add thread for thread list. (rv:%d)\n", rv);
			jthread_destroy( thread):
			return -1;
		}

		thrmgr->nthread++;
	}

	return 1;
}

void jthrmgr_final( jthrmgr_t *thrmgr){
	int i;
	jthread_t *thread;

	if( thrmgr->threads){
		for( i = 0; i < thrmgr->threads->nthread; i++) {
			thread = ( jthread_t*)dlist_ptr_find_last_node_data( thrmgr->threads);
			if( thread) {
				dlist_ptr_del_node_by_data( ( void*)thread, sizeof( thread));
				jthread_destroy( thread);
			}
		}
	}

	dlist_ptr_destroy( thrmgr->threads);
	thrmgr->data = NULL;
}

void jthrmgr_destroy( jthrmgr_t *thrmgr){
	jthrmgr_final( thrmgr);
	free( thrmgr);
}

void jthrmgr_join( jthrmgr_t *thrmgr){
	int i;
	jthread_t *thread;

	for( i = 0; i < thrmgr->nthread; i++){
		thread = dlist_find_last_node_data( thrmgr->threads);
		pthread_join( thread->id, NULL);
	}
}

int jthrmgr_run( jthrmgr_t *thrmgr){

}

void jthrmgr_stop( jthrmgr_t *thrmgr){

}

