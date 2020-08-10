#ifndef __JEVTMGR_H__
#define __JEVTMGR_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

///////////////////////////////////////////////////
// typedef & macros for jevtmgr
///////////////////////////////////////////////////

#define BUF_MAX_LEN 1024

typedef int (*jevtmgr_handle_wakeup_f)(jevtmgr_t *evtmgr, int nwakeup);
typedef int (*jevtmgr_handle_event_f)(jevtmgr_t *evtmgr, jevent_t *event);
typedef int (*jevtmgr_notify_f)(jevtmgr_t *evtmgr, void *data);

///////////////////////////////////////////////////
// struct for jevtmgr
///////////////////////////////////////////////////

/// @struct jnoti_t
/// @brief evtmgr 에서 사용하는 notify 구조체
typedef struct jnoti_s jnoti_t;
struct jnoti_s {
	int kind;
	int flags;
	void *data;
	jevtmgr_notify_f callback;
};

/// @struct jevevnt_t
/// @brief evtmgr 에서 사용하는 event 구조체
typedef struct jevent_s jevent_t;
struct jevent_s {
	int fd;
	short event;
	/// 사용자 데이터
	void *data;
	/// callback function
	jevtmgr_handle_event_f callback;
};

/// @struct jevtmgr_t
/// @brief 이벤트를 다루기 위한 event manager 구조체
typedef struct jevtmgr_s jevtmgr_t;
struct jevtmgr_s{
	int is_alive; /**< event manager 동작 여부 */
	int timeout; /**< event manager 에서 event 를 기다리기 위한 시간 */
	int epoll_handle_fd; /**< event manager 에서 사용하는 epoll handle fd */
	struct epoll_event events[ BUF_MAX_LEN]; /**< epoll handle fd 에서 깨어난 events */
	void *user_data; /**< 사용자 데이터 */
	jevtmgr_handle_wakeup_f handle_wakeup; /**< event 가 깨어났을 때 호출되는 callback function */
};

///////////////////////////////////////////////////
// local functions for jevtmgr
///////////////////////////////////////////////////

jevtmgr_t *jevtmgr_create( size_t num);
int jevtmgr_init( jevtmgr_t *evtmgr);
void jevtmgr_final( jevtmgr_t *evtmgr);
void jevtmgr_destroy( jevtmgr_t *evtmgr);
void jevtmgr_join( jevtmgr_t *evtmgr);

int jevtmgr_run( jevtmgr_t *evtmgr);
int jevtmgr_notify( jevtmgr_t *evtmgr, int kind, void *data, jevtmgr_notify_f callback);
int jevtmgr_start( jevtmgr_t *evtmgr);
int jevtmgr_stop( jevtmgr_t *evtmgr);

int jevtmgr_add_event( jevtmgr_t *evtmgr, jevent_t *event, short evtype, void *data, jevtmgr_handle_event_f callback);
int jevtmgr_remove_event( jevtmgr_t *evtmgr, jevent_t *event);
int jevtmgr_change_event( jevtmgr_t *evtmgr, jevent_t *event);

#endif // #ifndef __JEVTMGR_H__

