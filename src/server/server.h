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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "jmp.h"

#define HDR_MAX_LEN 20
#define BUF_MAX_LEN 1024
#define MSG_QUEUE_NUM 10
#define SERVER_PORT 8000

#define SPLICE_F_MOVE 0x01
#define SPLICE_F_NONBLOCK 0x02


/// @struct buf_t
/// @brief client 의 요청을 담기위한 버퍼 구조체
typedef struct buf_s buf_t;
struct buf_s{
	/// 버퍼 데이터
	char *data;
	/// 버퍼 수신 바이트
	int bytes;
};

/// @struct server_t
/// @brief client 의 요청에 따른 응답을 처리하기 위한 구조체
typedef struct server_s server_t;
struct server_s{
	/// server tcp socket file descriptor
	int fd;
	/// server socket address
	struct sockaddr_in addr;
};


server_t* server_init();
void server_destroy( server_t *server);
void server_conn( server_t *server);
int server_process_data( server_t *server, int client_fd);
bool server_recv_data( server_t *server, int client_fd, void *data);
bool server_send_data( server_t *server, int client_fd, void *data);
bool server_buf_data_init( buf_t *buffer);
void server_buf_data_destroy( buf_t *buffer);
void server_buf_clear( buf_t *buffer);

#endif

