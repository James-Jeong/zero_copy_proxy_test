#pragma once
#ifndef __DLIST_INT_H__
#define __DLIST_INT_H__

#include "common.h"

#define LIST_MAX_LEN 1024


/// @struct buf_t
/// @brief proxy 에서 thread 별 user buffer 관리를 위한 구조체
typedef struct node_int_s node_int_t;
struct node_int_s{
	/// 사용자 정의 data
	int data;
	/// 이전 노드 객체
	struct node_int_s *prev;
	/// 다음 노드 객체
	struct node_int_s *next;
};

/// @struct buf_t
/// @brief proxy 에서  위한 구조체
typedef struct dlist_int_s dlist_int_t;
struct dlist_int_s{
	/// 리스트 노드 객체 개수
	int length;
	/// 리스트 첫번째 노드 객체 (dummy)
	node_int_t head[ 1];
	/// 리스트 마지막 노드 객체 (dummy)
	node_int_t tail[ 1];
	/// 사용자 정의 data
	void *data;
};


dlist_int_t* dlist_int_create();
void dlist_int_init( dlist_int_t *list);
void dlist_int_destroy( dlist_int_t *list);
int dlist_int_add_node( dlist_int_t *list, int data);
int dlist_int_del_node_by_node( dlist_int_t *list, node_int_t *target);
int dlist_int_del_node_by_data( dlist_int_t *list, int data);
node_int_t* dlist_int_find_node_by_node( dlist_int_t *list, node_int_t *target);
int dlist_int_find_node_data_by_node( dlist_int_t *list, node_int_t *target);
node_int_t* dlist_int_find_node_by_data( dlist_int_t *list, int data);
int dlist_int_find_first_node_data( dlist_int_t *list);
int dlist_int_find_last_node_data( dlist_int_t *list);

node_int_t* node_int_create( int data);
void node_int_destroy( node_int_t *node);

#endif

