#include "server.h"

/**
 * @mainpage Project : zero copy server test
 * @section intro 소개
 *    - 차후 계획한 개인 프로젝트를 위해 미리 제작 
 * @section  CreateInfo 작성 정보
 *    - 작성자 :   정동욱
 *    - 작성일 :   2020/07/10
 * @subsection exec 실행 방법 및 인수 설명
 *    - 실행 방법\n
 *      서버       : SERVER/server <ip> <port>
 *      클라이언트 : CLIENT/client <hostname> <server ip> <server port>
 */


// -----------------------------------------

static int is_finish = false;
static int is_error = false;

// -----------------------------------------

static void server_signal_handler( int sig){
	printf("	| ! Server : server will be finished (sig:%d)\n", sig);
	is_finish = true;
	signal( sig, SIG_DFL);
}

static void server_set_signal( int sig_type){
	signal( sig_type, server_signal_handler);
}

/**
 * @fn static void server_transc_clear( transc_t *transc)
 * @brief transc 구조체 객체를 초기화하는 함수
 * @return void
 * @param transc 초기화하기 위한 transc_t 구조체 변수
 */
static void server_transc_clear( transc_t *transc){
	transc->is_recv_header = 0;
	transc->is_recv_body = 0;
	transc->is_send_header = 0;
	transc->is_send_body = 0;
	transc->length = 0;
	transc->recv_bytes = 0;
	transc->send_bytes = 0;
	memset( transc->read_hdr_buf, '\0', MSG_HEADER_LEN);
	memset( transc->read_body_buf, '\0', BUF_MAX_LEN);
	memset( transc->write_hdr_buf, '\0', MSG_HEADER_LEN);
	memset( transc->write_body_buf, '\0', BUF_MAX_LEN);
	transc->data = NULL;
}

/**
 * @fn static uint32_t server_transc_get_msg_length( transc_t *transc)
 * @brief 전달받은 메시지를 일부만 decode 해서 메시지의 총 길이(Header + Body)를 구하는 함수
 * @return 메시지 길이
 * @param transc 메시지의 길이를 구하기 위한 transc_t 구조체 변수
 */
static uint64_t server_transc_get_msg_length( transc_t *transc){
	if( ( strlen( transc->read_hdr_buf) == 0)){
		return -1;
	}

	// 1 byte = 8 bits
	// len size = 3 bytes
	uint8_t *data = ( uint8_t*)( transc->read_hdr_buf);

	// Big endian
//	uint32_t msg_len_b = ( ( ( int)( data[ 1])) << 16) + ( ( ( int)( data[ 2])) << 8) + data[ 3];

	// Little endian
	uint32_t msg_len_l = ( ( ( int)( data[ 3])) << 16) + ( ( ( int)( data[ 2])) << 8) + data[ 1];

//	printf("msg_len_b : %d\n", msg_len_b);
//	printf("msg_len_l : %d\n", msg_len_l); 

//	int i;
//	for( i = 0; i < 20; i++){
//		printf("%d = %d\n", i, ( uint8_t)data[i]); 
//	}	

	//return msg_len_b;
	return msg_len_l;
}

/**
 * @fn static int server_set_fd_nonblock( int fd)
 * @brief file descriptor 를 block 되지 않게(비동기로) 설정하는 함수
 * @return 정상적으로 설정되면 NORMAL, 비정상적이면 FD_ERR 반환
 * @param fd 확인할 server file descriptor
 */
static int server_set_fd_nonblock( int fd){
	int rv = 0;

	if( ( rv = fcntl( fd, F_GETFL, 0)) < 0){
		printf("	| ! Server : fcntl error (F_GETFL)\n");
		return FD_ERR;
	}

	if( ( rv = fcntl( fd, F_SETFL, rv | O_NONBLOCK)) < 0){
		printf("	| ! Server : fcntl error (F_SETFL & O_NONBLOCK)\n");
		return FD_ERR;
	}

	return NORMAL;
}

/**
 * @fn static int server_check_fd( int fd)
 * @brief src file descriptor 가 연결되어 있는지 확인
 * @return 정상 연결되어 있으면 NORMAL, 비정상 연결이면 FD_ERR 반환
 * @param fd 연결을 확인할 file descriptor
 */
static int server_check_fd( int fd){
	int error = 0;
	socklen_t err_len = sizeof( error);
	if( getsockopt( fd, SOL_SOCKET, SO_ERROR, &error, &err_len) < 0){
		return FD_ERR;
	}
	return NORMAL;
}

/**
 * @fn static int server_recv_data( transc_t *transc, int fd)
 * @brief client 가 server 로 데이터를 보낼 때, server 에서 user copy 로 수신하기 위한 함수
 * @return 열거형 참고
 * @param transc user copy 처리를 위한 transc_t 객체
 * @param fd 연결된 client file descriptor
 */
static int server_recv_data( transc_t *transc, int fd){
	char temp_read_hdr_buf[ MSG_HEADER_LEN];
	char temp_read_body_buf[ BUF_MAX_LEN];
	int recv_bytes = 0;
	int body_len = 0;
	int body_index = 0;

	// 메시지 받는거 인덱스 관리해서 받아야 한다.
	// 처음에 메시지 20 바이트 먼저 받고, 메시지 길이 얻어내서 바디 길이(msglen - 20)만큼 더 받는다.
	// 1. Recv header with <read> function
	if( ( transc->is_recv_header == 0) && ( transc->is_recv_body == 0)){ // recv header
		memset( transc->read_hdr_buf, '\0', MSG_HEADER_LEN);
		recv_bytes = read( fd, temp_read_hdr_buf, MSG_HEADER_LEN - ( transc->recv_bytes));
		if( recv_bytes < 0){
			if( ( errno == EAGAIN) || ( errno == EWOULDBLOCK)){
				//printf("        | @ Server : EAGAIN\n");
				return ERRNO_EAGAIN;
			}
			else if( errno == EINTR){
				printf("        | ! Server : Interrupted (in recv msg header) (fd:%d)\n", fd);
				return INTERRUPT;
			}
			else{
				printf("	| ! Server : read error (errno:%d) (in recv msg header) (fd:%d)\n", errno, fd);
				return NEGATIVE_BYTE;
			}
		}
		else if( recv_bytes == 0){
			printf("	| ! Server : read 0 byte (in recv msg header) (fd:%d)\n", fd);
			return ZERO_BYTE;
		}
		else{
			memcpy( &transc->read_hdr_buf[ transc->recv_bytes], temp_read_hdr_buf, recv_bytes);
			transc->recv_bytes += recv_bytes;

			if( transc->recv_bytes == MSG_HEADER_LEN){
				// If server recv Header all, then get message length
				transc->length = server_transc_get_msg_length( transc);
				body_len = transc->length - MSG_HEADER_LEN;
				if( body_len <= 0){
					printf("	| ! Server : msg body length is 0 (in recv msg header) (fd:%d)\n", fd);
					return BUF_ERR;
				}

				// Check success to recv & decode header
				if( transc->length > 0){
					transc->is_recv_header = 1;
				}
				else{
					return BUF_ERR;
				}
			}
			else{
				printf("	| ! Server : recv overflow error (in recv msg header) (fd:%d)\n", fd);
				return BUF_ERR;
			}
		}
	}
	else if( ( transc->is_recv_header == 0) && ( transc->is_recv_body == 1)){ // error
		printf("	| ! Server : recv unknown error, not recv hdr but recv body? (in recv msg header) (fd:%d)\n", fd);
		return UNKNOWN;
	}

	// 2. Recv body with <read> function
	if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 0)){ // If success to recv header, then recv body
		// The transc->recv_bytes is MSG_HEADER_LEN (20)
		recv_bytes = read( fd, temp_read_body_buf, transc->length - ( transc->recv_bytes));
		if( recv_bytes < 0){
			if( ( errno == EAGAIN) || ( errno == EWOULDBLOCK)){
				//printf("        | @ Server : EAGAIN\n");
				return ERRNO_EAGAIN;
			}
			else if( errno == EINTR){
				printf("        | ! Server : Interrupted (in read msg body) (fd:%d)\n", fd);
				return INTERRUPT;
			}
			else{
				printf("    | ! Server : read error (errno:%d) (in read msg body) (fd:%d)\n", errno, fd);
				return NEGATIVE_BYTE;
			}
		}
		else if( recv_bytes == 0){
			printf("    | ! Server : read 0 byte (in read msg body) (fd:%d)\n", fd);
			return ZERO_BYTE;
		}
		else{
			body_index = transc->recv_bytes - MSG_HEADER_LEN;
			memcpy( &transc->read_body_buf[ body_index], temp_read_body_buf, recv_bytes);
			transc->recv_bytes += recv_bytes;

			if( transc->recv_bytes == transc->length){
				transc->is_recv_body = 1;
			}
		}
	}

	if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 1)){
		printf("        | @ Server : Recv the msg (bytes : %d) (fd : %d)\n", transc->recv_bytes, fd);
	}

	//printf("        | ! Server : Fail to recv the msg (fd : %d)\n", fd);
	return NORMAL;
}

/**
 * @fn static int server_send_data( transc_t *transc, int fd)
 * @brief Server 가 Client 로 데이터를 보낼 때, Server 에서 zero copy 로 송신하기 위한 함수
 * @return 열거형 참고
 * @param transc user copy 처리를 위한 transc_t 객체
 * @param fd 연결된 client file descriptor
 */
static int server_send_data( transc_t *transc, int fd){
	if( fd < 0){
		printf("        | ! Server : Fail to get client_fd in server_send_data (fd:%d)\n", fd);
		return FD_ERR;
	}


	int write_bytes = 0;
	int body_len = 0;
	int body_index = 0;

	// 1. Send header with <write> function
	if( ( transc->is_send_header == 0) && ( transc->is_send_body == 0)){ // send header
		if( transc->send_bytes < MSG_HEADER_LEN){
			memset( transc->write_hdr_buf, '\0', MSG_HEADER_LEN);

			if( transc->send_bytes == 0){
				memcpy( transc->write_hdr_buf, transc->read_hdr_buf, MSG_HEADER_LEN);
			}
			else if( transc->send_bytes > 0){
				memcpy( &transc->write_hdr_buf[ transc->send_bytes], &transc->read_hdr_buf[ transc->send_bytes], MSG_HEADER_LEN - transc->send_bytes);
			}
			else{
				printf("    | ! Server : unknown error, transc->send_bytes is negative in server_send_data (fd:%d)\n", fd);
				return UNKNOWN;
			}
		}
		else{
			printf("    | ! Server : transc->send_bytes error, not sended header, nevertheless send_bytes is not less than MSG_HEADER_LEN in server_send_data (fd:%d)\n", fd);
			return UNKNOWN;
		}

		if( ( write_bytes = write( fd, transc->write_hdr_buf, MSG_HEADER_LEN - transc->send_bytes)) <= 0){
			printf("        | ! Server : Fail to write msg (fd:%d)\n", fd);
			if( errno == EAGAIN || errno == EWOULDBLOCK){
				return ERRNO_EAGAIN;
			}
			return NEGATIVE_BYTE;
		}

		transc->send_bytes += write_bytes;
		if( transc->send_bytes == MSG_HEADER_LEN){
			transc->is_send_header = 1;
			transc->recv_bytes -= write_bytes;
		}
	}
	else if( ( transc->is_send_header == 0) && ( transc->is_send_body == 1)){ // error
		printf("	| ! Server : send unknown error, not send hdr but send body? (in recv msg header) (fd:%d)\n", fd);
		return UNKNOWN;
	}

	// 2. Send body with <write> function
	if( ( transc->is_send_header == 1) && ( transc->is_send_body == 0)){
		body_len = transc->length - MSG_HEADER_LEN;
		if( transc->send_bytes < transc->length){
			memset( transc->write_body_buf, '\0', body_len);

			if( transc->send_bytes == MSG_HEADER_LEN){
				memcpy( transc->write_body_buf, transc->read_body_buf, body_len);
			}
			else if( transc->send_bytes > MSG_HEADER_LEN){
				body_index = transc->send_bytes - MSG_HEADER_LEN;
				memcpy( &transc->write_body_buf[ body_index], &transc->read_body_buf[ body_index], body_len - transc->send_bytes);
			}
			else{
				printf("    | ! Server : unknown error, transc->send_bytes is negative in server_send_data (fd:%d)\n", fd);
				return UNKNOWN;
			}
		}
		else{
			printf("    | ! Server : transc->send_bytes error, not sent header, nevertheless send_bytes is not less than MSG_HEADER_LEN in server_send_data (fd:%d)\n", fd);
			return UNKNOWN;
		}

		if( ( write_bytes = write( fd, transc->write_body_buf, transc->recv_bytes)) <= 0){
			if( errno == EAGAIN || errno == EWOULDBLOCK){
				return ERRNO_EAGAIN;
			}

			printf("        | ! Server : Fail to write msg\n");
			return NEGATIVE_BYTE;
		}
		else{
			transc->recv_bytes -= write_bytes;
			transc->send_bytes += write_bytes;

			if( transc->recv_bytes == 0){
				transc->is_send_body = 1;
			}
		}

		if( ( transc->is_send_header == 1) && ( transc->is_send_body == 1)){
			printf("        | @ Server : Send the msg (bytes : %d) (fd : %d)\n", transc->send_bytes, fd);
		}
	}

	//printf("        | ! Server : Fail to send the msg (fd : %d)\n", fd);
	return NORMAL;
}

/**
 * @fn static int server_process_data( server_t *server, int fd)
 * @brief Server 가 Client 로 데이터를 보낼 때, Server 에서 메시지 송수신을 처리하기 위한 함수
 * @return 열거형 참고
 * @param server 서버의 정보를 담고 있는 server_t 구조체 객체
 * @param fd 연결된 client file descriptor
 */
static int server_process_data( server_t *server, int fd){
	int i, event_count = 0;
	int read_rv = 0;
	int send_rv = 0;
	transc_t transc[ 1];

	printf("        | @ Server : waiting...\n");
	while( 1){
		if( ( server_check_fd( fd) == FD_ERR)){
			printf("	| @ Server : connection is ended (client:%d <-> server)\n", fd);
			break;
		}

		if( ( transc->is_send_header == 1) && ( transc->is_send_body == 1)){
			server_transc_clear( transc);
		}

		event_count = epoll_wait( server->epoll_handle_fd, server->events, BUF_MAX_LEN, TIMEOUT);
		if( event_count < 0){
			printf("    | ! Server : epoll_wait error\n");
			break;
		}
		else if( event_count == 0){
			printf("    | ! Server : epoll_wait timeout\n");
			continue;
		}

		for( i = 0; i < event_count; i++){
			if( server->events[ i].data.fd == server->fd){
			}
			else if( server->events[ i].data.fd == fd){
				if( transc->is_recv_header == 0 && transc->is_recv_body == 0){
					read_rv = server_recv_data( transc, fd);
					if( read_rv <= ZERO_BYTE){
						printf("	| ! Server : disconnected\n");
						printf("	| ! Server : socket closed\n");
						close( fd);
					}
				}

				if( transc->is_recv_header == 1 && transc->is_recv_body == 0){
					read_rv = server_recv_data( transc, fd);
					if( read_rv <= ZERO_BYTE){
						printf("	| ! Server : disconnected\n");
						printf("	| ! Server : socket closed\n");
						close( fd);
					}
				}

				if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 1)){
					if( ( send_rv = server_send_data( transc, fd)) <= ZERO_BYTE){
						printf("        | ! Server : Fail to send msg (fd:%d)\n", fd);
					}
				}

				if( ( transc->is_send_header == 1) && ( transc->is_send_body == 0)){
					if( ( send_rv = server_send_data( transc, fd)) <= ZERO_BYTE){
						printf("        | ! Server : Fail to send msg (fd:%d)\n", fd);
					}
				}
			}
		}
	}

	return NORMAL;
}

/**
 * @fn static void* server_detect_finish( void *data)
 * @brief Server 에서 사용하는 메모리를 해제하기 위한 함수
 * @return None
 * @param data Thread 매개변수, 현재 사용되는(할당된) Server 객체
 */
static void* server_detect_finish( void *data){
	server_t *server = ( server_t*)( data);

	while( 1){
		//if( server_check_fd( server->dst_fd) == FD_ERR){
		//	server_destroy( server);
		//	break;
		//}

		if( is_error == true){
			break;
		}

		if( is_finish == true){
			server_destroy( server);
			is_error = true;
			break;
		}
		
		sleep( 1);
	}
}

/**
 * @fn static int server_handle_finish( server_t *server)
 * @brief Server 가 종료되기 위해 종료 여부를 확인하는 Thread 구동하기 위한 함수
 * @return Thread 생성 시 오류 발생 여부 반환
 * @param server 현재 사용되는(할당된) Server 객체
 */
static int server_handle_finish( server_t *server){
	pthread_t thread;

	if( ( pthread_create( &thread, NULL, server_detect_finish, server)) != 0){
		printf("	| ! Server : Fail to create a thread for detecting is_finish var\n");
		return PTHREAD_ERR;
	}

	if( ( pthread_detach( thread)) != 0){
		printf("	| ! Server : Fail to detach the thread for detecting is_finish var\n");
		return PTHREAD_ERR;
	}

	return NORMAL;
}


// -----------------------------------------

/**
 * @fn server_t* server_init()
 * @brief server 객체를 생성하고 초기화하는 함수
 * @return 생성된 server 객체
 */
server_t* server_init( char **argv){
	int rv;
	server_t *server = ( server_t*)malloc( sizeof( server_t));

	if ( server == NULL){
		printf("	| ! Server : Fail to allocate memory\n");
		return NULL;
	}

	server_set_signal( SIGINT);
	if( ( server_handle_finish( server)) == PTHREAD_ERR){
		free( server);
		return NULL;
	}

	memset( &server->addr, 0, sizeof( struct sockaddr));
	server->addr.sin_family = AF_INET;
	server->addr.sin_addr.s_addr = inet_addr( argv[1]);
	server->addr.sin_port = htons( atoi( argv[2]));

	if( ( server->fd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("	| ! Server : Fail to open socket\n");
		free( server);
		return NULL;
	}

	int reuse = 1;
	if( setsockopt( server->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse))){
		printf("	| ! Server : Fail to set the socket's option\n");
		free( server);
		return NULL;
	}

	if( bind( server->fd, ( struct sockaddr*)( &server->addr), sizeof( server->addr)) < 0){
		printf("	| ! Server : Fail to bind socket\n");
		close( server->fd);
		free( server);
		return NULL;
	}

	if( listen( server->fd, MSG_QUEUE_NUM)){
		printf("	| ! Server : listen error\n");
		close( server->fd);
		free( server);
		return NULL;
	}

	rv = server_set_fd_nonblock( server->fd);
	if( rv < 0){
		close( server->fd);
		free( server);
		return NULL;
	}

	if( ( server->epoll_handle_fd = epoll_create( BUF_MAX_LEN)) < 0){
		printf("    | ! Server : Fail to create epoll handle fd\n");
		close( server->fd);
		free( server);
		return ;
	}

	struct epoll_event server_event;
	server_event.events = EPOLLIN;
	server_event.data.fd = server->fd;
	if( ( epoll_ctl( server->epoll_handle_fd, EPOLL_CTL_ADD, server->fd, &server_event)) < 0){
		printf("    | ! Server : Fail to add epoll server event\n");
		close( server->fd);
		free( server);
		return NULL;
	}


	printf("	| @ Server : Success to create a object\n");
	printf("	| @ Server : Welcome\n\n");
	return server;
}

/**
 * @fn void server_destroy( server_t *server)
 * @brief server 객체를 삭제하기 위한 함수
 * @return void
 * @param server 삭제하기 위한 server 객체
 */
void server_destroy( server_t *server){
	close( server->fd);
	free( server);

	printf("	| @ Server : Success to destroy the object\n");
	printf("	| @ Server : BYE\n\n");
}

/**
 * @fn int server_conn( server_t *server)
 * @brief client 와 연결되었을 때 데이터를 수신하고 데이터를 처리하기 위한 함수
 * @return client 와 정상 연결 여부 
 * @param server 데이터 처리를 위한 server 객체
 */
int server_conn( server_t *server){
	if( server_check_fd( server->fd) == FD_ERR){
		return SOC_ERR;
	}


	int rv = 0;
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof( client_addr);
	memset( &client_addr, 0, client_addr_len);

	int client_fd = accept( server->fd, ( struct sockaddr*)( &client_addr), ( socklen_t*)( &client_addr_len));
	if( client_fd < 0){
//		printf("	| ! Server : accept error\n");
		return FD_ERR;
	}

	rv = server_set_fd_nonblock( client_fd);
	if( rv < NORMAL){
		return FD_ERR;
	}

	if( ( server->epoll_handle_fd = epoll_create( BUF_MAX_LEN)) < 0){
		printf("    | ! Proxy : Fail to create epoll handle fd\n");
		return OBJECT_ERR;
	}

	struct epoll_event client_event;
	client_event.events = EPOLLIN;
	client_event.data.fd = client_fd;
	if( ( epoll_ctl( server->epoll_handle_fd, EPOLL_CTL_ADD, client_fd, &client_event)) < 0){
		printf("    | ! Server : Fail to add epoll client event\n");
		return OBJECT_ERR;
	}

	rv = server_process_data( server, client_fd);
	if( rv < NORMAL){
		printf("	| ! Server : process end\n");
		return OBJECT_ERR;
	}

	return NORMAL;
}


// -----------------------------------------

/**
 * @fn int main( int argc, char **argv)
 * @brief server 구동을 위한 main 함수
 * @return int
 * @param argc 매개변수 개수
 * @param argv ip 와 포트 정보
 */
int main( int argc, char **argv){
	if( argc != 3){
		printf("	| ! need param : ip port\n");
		return UNKNOWN;
	}

	int rv;
	server_t *server = server_init( argv);
	if( server == NULL){
		printf("	| ! Server : Fail to initialize\n");
		return UNKNOWN;
	}

	while( 1){
		usleep( 100);
		rv = server_conn( server);
		if( rv <= FD_ERR){
			if( rv == SOC_ERR){
				printf("	| ! Server : server fd closed\n");
				break;
			}
			continue;
		}
	}

	if( is_finish == false){
		printf("	| @ Server : Please press <ctrl+c> to exit\n");
		is_error = true;
		while( 1){
			if( is_finish == true){
				break;
			}
			sleep( 1);
		}
		server_destroy( server);
	}
	else{
		while( 1){
			if( is_error == true){
				break;
			}
			sleep( 1);
		}
	}

	return NORMAL;
}

