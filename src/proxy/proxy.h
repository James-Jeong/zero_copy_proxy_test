#ifndef __PROXY_H__
#define __PROXY_H__

#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#include "common.h"
#include "jpool.h"
#include "transc.h"
#include "dlist_int.h"
#include "dlist_ptr.h"

#define MSG_QUEUE_NUM 10
#define TIMEOUT 10000
#define THREAD_NUM 2

#define SPLICE_F_MOVE 0x01
#define SPLICE_F_NONBLOCK 0x02


/// @struct proxy_t
/// @brief client 의 요청에 따른 응답을 처리하기 위한 구조체
typedef struct proxy_s proxy_t;
struct proxy_s{
	/// proxy tcp socket file descriptor
	int fd;
	/// destination tcp socket file descriptor
	int dst_fd;
	/// proxy socket address
	struct sockaddr_in addr;
	/// proxy thread pool object
	jpool_t *jpool;
	/// proxy epoll handle file descriptor
	int epoll_handle_fd;
	/// proxy epoll event management structure
	struct epoll_event events[ BUF_MAX_LEN];
	/// server local ip
	char *local_ip;
	/// server local port
	int local_port;
	/// server dst ip
	char *dst_ip;
	/// server dst port
	int dst_port;
	/// accepted source file descriptor management data structure
	dlist_int_t fd_list[ 1];
	/// work data management data structure
	dlist_ptr_t work_list [ 1];
};


proxy_t* proxy_init( char **argv);
void proxy_destroy( proxy_t *proxy);
void proxy_handle_req( proxy_t *proxy);

#endif

