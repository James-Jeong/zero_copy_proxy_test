#pragma once
#ifndef __DLIST_PTR_H__
#define __DLIST_PTR_H__

#include "common.h"
#include "jpool.h"

#define NODE_MAX_SIZE 1000
#define LIST_MAX_SIZE 1024


/// @struct buf_t
/// @brief proxy 에서 thread 별 user buffer 관리를 위한 구조체
typedef struct node_ptr_s node_ptr_t;
struct node_ptr_s{
	/// 이전 노드 객체
	struct node_ptr_s *prev;
	/// 다음 노드 객체
	struct node_ptr_s *next;
	/// 사용자 정의 data
	void *data;
};

/// @struct buf_t
/// @brief proxy 에서  위한 구조체
typedef struct dlist_ptr_s dlist_ptr_t;
struct dlist_ptr_s{
	/// 리스트 노드 객체 개수
	int length;
	/// 리스트 첫번째 노드 객체 (dummy)
	node_ptr_t head[ 1];
	/// 리스트 마지막 노드 객체 (dummy)
	node_ptr_t tail[ 1];
	/// 사용자 정의 data
	void *data;
};


dlist_ptr_t* dlist_ptr_create();
void dlist_ptr_init( dlist_ptr_t *list);
void dlist_ptr_destroy( dlist_ptr_t *list);
int dlist_ptr_add_node( dlist_ptr_t *list, void *data);
int dlist_ptr_del_node_by_node( dlist_ptr_t *list, node_ptr_t *target);
int dlist_ptr_del_node_by_data( dlist_ptr_t *list, void *data);
node_ptr_t* dlist_ptr_find_node_by_node( dlist_ptr_t *list, node_ptr_t *target);
void* dlist_ptr_find_node_data_by_node( dlist_ptr_t *list, node_ptr_t *target);
void* dlist_ptr_find_node_data_by_data( dlist_ptr_t *list, void *data);
node_ptr_t* dlist_ptr_find_node_by_data( dlist_ptr_t *list, void *data);
void* dlist_ptr_find_first_node_data( dlist_ptr_t *list);
void* dlist_ptr_find_last_node_data( dlist_ptr_t *list);
node_ptr_t* dlist_ptr_find_last_node( dlist_ptr_t *list);
node_ptr_t* dlist_ptr_get_prev_node( dlist_ptr_t *list, node_ptr_t *target);
void dlist_ptr_print_all( dlist_ptr_t *list);

#endif

