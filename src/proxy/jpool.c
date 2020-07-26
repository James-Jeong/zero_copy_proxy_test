#include "jpool.h"

/**
 * @fn static jpool_work_t *jpool_work_init( func_pointer func, void *arg)
 * @brief thread pool 에 사용될 work 를 등록하는 함수
 * @return 등록된 work
 * @param func 실행할 함수
 * @param arg 실행할 함수에 사용될 매개변수
 */
static jpool_work_t *jpool_work_init( func_pointer func, void *arg){
	if ( func == NULL){
		return NULL;
	}

	jpool_work_t *work = malloc( sizeof( jpool_work_t));
	work->func = func;
	work->arg = arg;
	work->next = NULL;
	return work;
}

/**
 * @fn static void jpool_work_destroy( jpool_work_t *work)
 * @brief thread pool 에 사용될 work 를 삭제하는 함수
 * @return void
 * @param work 삭제할 work
 */
static void jpool_work_destroy( jpool_work_t *work){
	if( work == NULL){
		return;
	}

	free( work);
}

/**
 * @fn static jpool_work_t* jpool_work_get( jpool_t *jpool)
 * @brief thread pool 에 등록된 work 를 가져오는 함수
 * @return 등록된 work
 * @param jpool work 를 가져올 쓰레드 풀 객체
 */
static jpool_work_t* jpool_work_get( jpool_t *jpool){
	jpool_work_t *work;

	if( jpool == NULL){
		return NULL;
	}

	work = jpool->work_first;
	if( work == NULL){
		return NULL;
	}

	if( work->next == NULL){
		jpool->work_first = NULL;
		jpool->work_last = NULL;
	}
	else{
		jpool->work_first = work->next;
	}

	return work;
}

/**
 * @fn static void *jpool_worker( void *arg)
 * @brief work 에 등록된 함수를 실행하는 함수
 * @return void
 * @param arg work 가진 함수에 사용될 매개변수
 */
static void *jpool_worker( void *arg){
	jpool_t *jpool = arg;
	jpool_work_t *work;

	while(1){
		pthread_mutex_lock(&(jpool->work_mutex));

		while( jpool->work_first == NULL && !jpool->stop){
			pthread_cond_wait( &( jpool->work_cond), &( jpool->work_mutex));
		}

		if( jpool->stop){
			break;
		}

		work = jpool_work_get( jpool);
		jpool->working_cnt++;
		pthread_mutex_unlock( &( jpool->work_mutex));

		if( work != NULL){
			work->func( work->arg);
			jpool_work_destroy( work);
		}

		pthread_mutex_lock( &( jpool->work_mutex));
		jpool->working_cnt--;

		if( !jpool->stop && jpool->working_cnt == 0 && jpool->work_first == NULL){
			pthread_cond_signal( &( jpool->working_cond));
		}

		pthread_mutex_unlock( &( jpool->work_mutex));
	}

	jpool->thread_cnt--;
	pthread_cond_signal( &( jpool->working_cond));
	pthread_mutex_unlock( &( jpool->work_mutex));

	return NULL;
}

// -----------------------------------------

/**
 * @fn jpool_t* jpool_init( size_t num)
 * @brief thread pool 객체를 생성하고 초기화하는 함수
 * @return 생성된 thread pool 객체
 * @param num 생성될 쓰레드 개수
 */
jpool_t* jpool_init( size_t num){
	jpool_t *jpool;
	pthread_t thread;
	size_t i;

	if( num == 0){
		num = 2;
	}

	jpool = calloc(1, sizeof(*jpool));
	if( jpool == NULL){
		printf("	| ! Fail to create a jpool\n");
		return NULL;
	}

	jpool->thread_cnt = num;
	jpool->work_first = NULL;
	jpool->work_last = NULL;

	if( (pthread_mutex_init( &( jpool->work_mutex), NULL)) != 0){
		free( jpool);
		printf("	| ! Fail to init work_mutex\n");
		return NULL;
	}
	if( (pthread_cond_init( &( jpool->work_cond), NULL)) != 0){
		pthread_mutex_destroy( &( jpool->work_mutex));
		free( jpool);
		printf("	| ! Fail to init work_cond\n");
		return NULL;
	}
	if( (pthread_cond_init( &( jpool->working_cond), NULL)) != 0){
		pthread_mutex_destroy( &( jpool->work_mutex));
		pthread_cond_destroy( &( jpool->work_cond));
		free( jpool);
		printf("	| ! Fail to init working_cond\n");
		return NULL;
	}

	for( i = 0; i < num; i++){
		if( ( pthread_create( &thread, NULL, jpool_worker, jpool)) != 0){
			pthread_mutex_destroy( &( jpool->work_mutex));
			pthread_cond_destroy( &( jpool->work_cond));
			pthread_cond_destroy( &( jpool->working_cond));
			free( jpool);
			printf("	| ! Fail to create a thread\n");
			return NULL;
		}
		pthread_detach( thread);
	}

	return jpool;
}

/**
 * @fn void jpool_destroy( jpool_t *jpool)
 * @brief thread pool 객체를 삭제하는 함수
 * @return void
 * @param jpool 삭제될 thread pool 객체
 */
void jpool_destroy( jpool_t *jpool){
	if(jpool == NULL){
		return;
	}

	jpool_work_t *work;
	jpool_work_t *work_temp;

	pthread_mutex_lock( &( jpool->work_mutex));
	work = jpool->work_first;
	while( work != NULL) {
		work_temp = work->next;
		jpool_work_destroy( work);
		work = work_temp;
	}

	jpool->stop = true;
	pthread_cond_broadcast( &( jpool->work_cond));
	pthread_mutex_unlock( &( jpool->work_mutex));

	jpool_wait( jpool);

	pthread_mutex_destroy( &( jpool->work_mutex));
	pthread_cond_destroy( &( jpool->work_cond));
	pthread_cond_destroy( &( jpool->working_cond));

	free( jpool);
}

/**
 * @fn bool jpool_add_work( jpool_t *jpool, func_pointer func, void *arg)
 * @brief thread pool 객체에 특정 함수를 실행할 work 를 등록하는 함수
 * @return 실패 여부
 * @param jpool work 를 추가할 thread pool 객체
 * @param func 실행할 함수
 * @param arg 실행할 함수에 사용될 매개변수
 */
bool jpool_add_work( jpool_t *jpool, func_pointer func, void *arg){
	if( jpool == NULL){
		return false;
	}

	jpool_work_t *work = jpool_work_init( func, arg);
	if( work == NULL){
		return false;
	}

	pthread_mutex_lock( &( jpool->work_mutex));

	if( jpool->work_first == NULL) {
		jpool->work_first = work;
		jpool->work_last = jpool->work_first;
	}
	else{
		jpool->work_last->next = work;
		jpool->work_last = work;
	}

	pthread_cond_broadcast( &( jpool->work_cond));
	pthread_mutex_unlock( &( jpool->work_mutex));

	return true;
}

/**
 * @fn void* jpool_wait( void *arg)
 * @brief 동작 중인 thread 가 있는지 확인하는 함수
 * @return void
 * @param jpool 확인에 사용될 thread pool 객체
 */
void jpool_wait( jpool_t *jpool){
	if( jpool == NULL){
		return;
	}

	pthread_mutex_lock( &( jpool->work_mutex));

	while(1){
		if( ( !jpool->stop && jpool->working_cnt != 0) || ( jpool->stop && jpool->thread_cnt != 0)) {
			pthread_cond_wait( &( jpool->working_cond), &( jpool->work_mutex));
		}
		else{
			break;
		}
	}

	pthread_mutex_unlock( &( jpool->work_mutex));
}

/**
 * @fn int jpool_create_work_data( int src_fd, int dst_fd, void *arg)
 * @brief work data 객체를 생성하고 데이터를 초기화하는 함수
 * @return void
 * @param src_fd source file desriptor
 * @param dst_fd destination file desriptor
 * @param arg work 에 사용될 데이터(proxy object)
 */
jpool_work_data_t* jpool_create_work_data( int src_fd,  int dst_fd, void *arg){
	jpool_work_data_t *work_data = ( jpool_work_data_t*)malloc(sizeof( jpool_work_data_t));
	work_data->src_fd = src_fd;
	work_data->dst_fd = dst_fd;
	work_data->arg = arg;
//	work_data->arg = malloc( sizeof( arg));
//	memcpy( work_data->arg, arg, sizeof( arg));
	return work_data;
}

/**
 * @fn void jpool_destroy_work_data( jpool_work_data_t *work_data)
 * @brief 생성된 jpool_work_data_t 구조체 객체의 메모리를 해제하는 함수
 * @return void
 * @param work_data 생성된 work data 객체
 */
void jpool_destroy_work_data( jpool_work_data_t *work_data){
	if( work_data){
		free( work_data);
	}
}

/**
 * @fn void jpool_set_work_data( jpool_work_data_t *work_data, int src_fd, int dst_fd, void *arg)
 * @brief work data 객체의 데이터를 세팅하는 함수
 * @return void
 * @param work_data 데이터가 세팅될 jpool_work_data_t 구조체 객체
 * @param src_fd source file desriptor
 * @param dst_fd destination file desriptor
 * @param arg work 에 사용될 데이터(proxy object)
 */
void jpool_set_work_data( jpool_work_data_t *work_data, int src_fd, int dst_fd, void *arg){
	if( work_data){
		work_data->src_fd = src_fd;
		work_data->dst_fd = dst_fd;
		work_data->arg = arg;
	}
}

