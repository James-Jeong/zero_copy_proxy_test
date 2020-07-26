#pragma once
#ifndef __TRANSC_H__
#define __TRANSC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MSG_HEADER_LEN 20
#define BUF_MAX_LEN 1024

typedef unsigned char byte;

/// @struct transc_t
/// @brief proxy 에서 thread 별 user transcfer 관리를 위한 구조체
typedef struct transc_s transc_t;
struct transc_s{
	/// 메시지 헤더 수신 여부
	int is_recv_header;
	/// 메시지 바디 수신 여부
	int is_recv_body;
	/// 메시지 헤더 송신 여부
	int is_send_header;
	/// 메시지 헤더 송신 여부
	int is_send_body;
	/// 전달 받은 메시지 length
	int length;
	/// 전달 받은 메시지의 바이트
	int recv_bytes;
	/// 보낸 메시지의 바이트
	int send_bytes;
	/// 전달 받은 메시지 헤더 데이터
	char read_hdr_buf[ MSG_HEADER_LEN];
	/// 보낸 메시지 헤더 데이터
	char write_hdr_buf[ MSG_HEADER_LEN];
	/// 사용자 정의 data
	void *data;
};

void transc_clear_data( transc_t *transc);
uint64_t transc_get_msg_length( transc_t *transc);

#endif

