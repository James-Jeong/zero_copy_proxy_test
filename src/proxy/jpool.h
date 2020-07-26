#ifndef __JPOOL_H__
#define __JPOOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "transc.h"
#include "pipe.h"

typedef void ( *func_pointer) ( void *arg);

/// @struct jpool_work_t
/// @brief thread pool 에서 작업을 등록하기 위한 work 구조체
typedef struct jpool_work_s jpool_work_t;
struct jpool_work_s {
	/// function pointer
	func_pointer func;
	/// parameter
	void *arg;
	/// next work
	struct jpool_work_s *next;
};

/// @struct jpool_work_data_t
/// @brief worker 의 작업 데이터를 위한 work 구조체
typedef struct jpool_work_data_s jpool_work_data_t;
struct jpool_work_data_s {
	/// source file descriptor
	int src_fd;
	/// destination file descriptor
	int dst_fd;
	/// message management object1 (for src->dst)
	transc_t ingoing_transc[1];
	/// message management object2 (for dst->src)
	transc_t outgoing_transc[1];
	/// pipe1 data (for src->dst zero copy)
	pipe_t _pipe1[1];
	/// pipe2 data (for dst->src zero copy)
	pipe_t _pipe2[1];
	/// 사용자 정의 data
	void *arg;
};

/// @struct jpool_t
/// @brief 여러 개의 클라이언트를 다루기 위한 thread pool 구조체
typedef struct jpool_s jpool_t;
struct jpool_s{
	/// 첫번째 work
	jpool_work_t *work_first;
	/// 마지막 work
	jpool_work_t *work_last;
	/// work critical 방지 변수
	pthread_mutex_t work_mutex;
	/// work 존재 유무를 위한 조건 변수
	pthread_cond_t work_cond;
	/// worker 의 동작 유무를 위한 조건 변수
	pthread_cond_t working_cond;
	/// 현재 동작 중인 work 개수
	size_t working_cnt;
	/// 현재 동작 중인 thread 개수
	size_t thread_cnt;
	/// 동작 중인 쓰레드를 모두 중지시키기 위한 flag
	bool stop;
};


jpool_t *jpool_init( size_t num);
void jpool_destroy( jpool_t *jpool);

bool jpool_add_work( jpool_t *jpool, func_pointer func, void *arg);
void jpool_wait( jpool_t *jpool);
jpool_work_data_t* jpool_create_work_data( int src_fd, int dst_fd, void *arg); 
void jpool_destroy_work_data( jpool_work_data_t *work_data); 
void jpool_set_work_data( jpool_work_data_t *work_data, int src_fd, int dst_fd, void *arg); 

#endif

