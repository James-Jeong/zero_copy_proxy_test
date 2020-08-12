#ifndef __JTHREAD_H__
#define __JTHREAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "dlist_ptr_l.h"

///////////////////////////////////////////////////
// typedef & macros for jthrmgr
///////////////////////////////////////////////////
#define MAX_NTHREAD 4

///////////////////////////////////////////////////
// struct for jthrmgr
///////////////////////////////////////////////////
typedef struct jthread_s jthread_t;
struct jthread_t{
	int id;
	int is_alive;
	void *data; /**< user data */
};

typedef struct jthrmgr_s jthrmgr_t;
struct jthrmgr_t{
	int nthread; /**< thread count */
	dlist_ptr_t *threads; /**< thread list */
	void *data; /**< user data */
};

///////////////////////////////////////////////////
// local functions for jthread
///////////////////////////////////////////////////
jthread_t *jthread_create();
int jthread_init( jthread_t *thread);
void jthread_final( jthread_t *thread);
void jthread_destroy( jthread_t *thread);
int jthread_run( jthread_t *thread);
void jthread_stop( jthread_t *thread);

///////////////////////////////////////////////////
// local functions for jthrmgr
///////////////////////////////////////////////////
jthrmgr_t *jthrmgr_create( size_t num);
int jthrmgr_init( jthrmgr_t *thrmgr);
void jthrmgr_final( jthrmgr_t *thrmgr);
void jthrmgr_destroy( jthrmgr_t *thrmgr);
void jthrmgr_join( jthrmgr_t *thrmgr);
int jthrmgr_run( jthrmgr_t *thrmgr);
void jthrmgr_stop( jthrmgr_t *thrmgr);

#endif // #ifndef __JTHREAD_H__
