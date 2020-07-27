#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>

#include "common.h"

#define MSG_HEADER_LEN 20
#define BUF_MAX_LEN 1024
#define MSG_QUEUE_NUM 10
#define SERVER_PORT 8000
#define TIMEOUT 10000

//#define SPLICE_F_MOVE 0x01
//#define SPLICE_F_NONBLOCK 0x02


/// @struct transc_t
/// @brief server 에서 user buffer 관리를 위한 구조체
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
	/// 전달 받은 메시지 바디 데이터
	char read_body_buf[ BUF_MAX_LEN];
	/// 보낸 메시지 헤더 데이터
	char write_hdr_buf[ MSG_HEADER_LEN];
	/// 보낸 메시지 바디 데이터
	char write_body_buf[ BUF_MAX_LEN];
	/// 사용자 정의 data
	void *data;
};

/// @struct server_t
/// @brief client 의 요청에 따른 응답을 처리하기 위한 구조체
typedef struct server_s server_t;
struct server_s{
	/// server tcp socket file descriptor
	int fd;
	/// server socket address
	struct sockaddr_in addr;
	/// server epoll handle file descriptor
	int epoll_handle_fd;
	/// server epoll event management structure
	struct epoll_event events[ BUF_MAX_LEN];
};


server_t* server_init();
void server_destroy( server_t *server);
int server_conn( server_t *server);

#endif

