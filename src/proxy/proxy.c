#include "proxy.h"

static int is_finish = false;
static int is_error = false;

// -----------------------------------------

static void proxy_signal_handler( int sig){
	printf("	| ! Proxy : proxy will be finished (sig:%d)\n", sig);
	is_finish = true;
	signal( sig, SIG_DFL);
}

static void proxy_set_signal( int sig_type){
	signal( sig_type, proxy_signal_handler);
}

/**
 * @fn static int proxy_set_fd_nonblock( int fd)
 * @brief file descriptor 를 block 되지 않게(비동기로) 설정하는 함수
 * @return 정상적으로 설정되면 NORMAL, 비정상적이면 FD_ERR 반환
 * @param fd 확인할 src file descriptor
 */
static int proxy_set_fd_nonblock( int fd){
	int rv = 0;

	if( ( rv = fcntl( fd, F_GETFL, 0)) < 0){
		printf("	| ! Proxy : fcntl error (F_GETFL)\n");
		return FD_ERR;
	}

	if( ( rv = fcntl( fd, F_SETFL, rv | O_NONBLOCK)) < 0){
		printf("	| ! Proxy : fcntl error (F_SETFL & O_NONBLOCK)\n");
		return FD_ERR;
	}

	return NORMAL;
}

/**
 * @fn static int proxy_check_fd( int fd)
 * @brief src file descriptor 가 연결되어 있는지 확인
 * @return 정상 연결되어 있으면 NORMAL, 비정상 연결이면 FD_ERR 반환
 * @param fd 확인할 src file descriptor
 */
static int proxy_check_fd( int fd){
	int error = 0;
	socklen_t err_len = sizeof( error);
	if( getsockopt( fd, SOL_SOCKET, SO_ERROR, &error, &err_len) < 0){
		return FD_ERR;
	}
	return NORMAL;
}

/**
 * @fn static int proxy_recv_data( transc_t *transc, int fd)
 * @brief src 가 proxy 로 데이터를 보낼 때, proxy 에서 zero copy 로 수신하기 위한 함수
 * @return 열거형 참고
 * @param transc user copy 처리를 위한 transc_t 객체
 * @param _pipe zero copy 처리를 위한 pipe_t 객체
 * @param fd 연결된 src file descriptor
 */
static int proxy_recv_data( transc_t *transc, pipe_t *_pipe, int fd){
	if( fd < 0){
		printf("        | ! Proxy : Fail to get src_fd in proxy_recv_data (fd:%d)\n", fd);
		return FD_ERR;
	}

	if( _pipe == NULL){
		printf("        | ! Proxy : Fail to get pipe in proxy_recv_data (fd:%d)\n", fd);
		return BUF_ERR;
	}


	char temp_read_hdr_buf[ MSG_HEADER_LEN];
	int body_len = 0;
	int recv_bytes = 0;

	// 메시지 받는거 인덱스 관리해서 받아야 한다.
	// 처음에 메시지 20 바이트 먼저 받고, 메시지 길이 얻어내서 바디 길이(msglen - 20)만큼 더 받는다.
	// 1. Recv header with <read> function
	if( ( transc->is_recv_header == 0) && ( transc->is_recv_body == 0)){ // recv header
		memset( transc->read_hdr_buf, '\0', MSG_HEADER_LEN);
		recv_bytes = read( fd, temp_read_hdr_buf, MSG_HEADER_LEN - ( transc->recv_bytes));
		if( recv_bytes < 0){
			if( ( errno == EAGAIN) || ( errno == EWOULDBLOCK)){
				//printf("        | @ Proxy : EAGAIN\n");
				return ERRNO_EAGAIN;
			}
			else if( errno == EINTR){
				printf("        | ! Proxy : Interrupted (in recv msg header) (fd:%d)\n", fd);
				return INTERRUPT;
			}
			else{
				printf("	| ! Proxy : read error (errno:%d) (in recv msg header) (fd:%d)\n", errno, fd);
				return NEGATIVE_BYTE;
			}
		}
		else if( recv_bytes == 0){
			printf("	| ! Proxy : read 0 byte (in recv msg header) (fd:%d)\n", fd);
			return ZERO_BYTE;
		}
		else{
			memcpy( &transc->read_hdr_buf[ transc->recv_bytes], temp_read_hdr_buf, recv_bytes);
			transc->recv_bytes += recv_bytes;

			if( transc->recv_bytes == MSG_HEADER_LEN){
				// If proxy recv Header all, then get message length
				transc->length = transc_get_msg_length( transc);
				body_len = transc->length - MSG_HEADER_LEN;
				if( body_len <= 0){
					printf("	| ! Proxy : msg body length is 0 (in recv msg header) (fd:%d)\n", fd);
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
				printf("	| ! Proxy : recv overflow error (in recv msg header) (fd:%d)\n", fd);
				return BUF_ERR;
			}
		}
	}
	else if( ( transc->is_recv_header == 0) && ( transc->is_recv_body == 1)){ // error
		printf("	| ! Proxy : recv unknown error, not recv hdr but recv body? (in recv msg header) (fd:%d)\n", fd);
		return UNKNOWN;
	}

	// 2. Recv body with <splice> function
	if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 0)){ // If success to recv header, then recv body
		// The transc->bytes is MSG_HEADER_LEN (20)
		recv_bytes = splice( fd, NULL, _pipe->fd[ 1], NULL, ( transc->length - ( transc->recv_bytes)), SPLICE_F_NONBLOCK | SPLICE_F_MOVE);
		if( recv_bytes < 0){
			if( ( errno == EAGAIN) || ( errno == EWOULDBLOCK)){
				//printf("        | @ Proxy : EAGAIN\n");
				return ERRNO_EAGAIN;
			}
			else if( errno == EINTR){
				printf("        | ! Proxy : Interrupted (in recv msg body) (fd:%d)\n", fd);
				return INTERRUPT;
			}
			else{
				printf("	| ! Proxy : recv error (errno:%d) (in recv msg body) (fd:%d)\n", errno, fd);
				return NEGATIVE_BYTE;
			}
		}
		else if( recv_bytes == 0){
			printf("	| ! Proxy : recv 0 byte (in recv msg body) (fd:%d)\n", fd);
			return ZERO_BYTE;
		}
		else{
			transc->recv_bytes += recv_bytes;

			if( transc->recv_bytes == transc->length){
				transc->is_recv_body = 1;
			}
		}
	}

	if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 1)){
		//printf("        | @ Proxy : Recv the msg (bytes : %d) (fd : %d)\n", transc->recv_bytes, fd);
	}

	//printf("        | ! Proxy : Fail to recv the msg (fd : %d)\n", fd);
	return NORMAL;
}

/**
 * @fn static int proxy_send_data( transc_t *transc, int fd)
 * @brief Proxy 가 Source 로 데이터를 보낼 때, Proxy 에서 zero copy 로 송신하기 위한 함수
 * @return 열거형 참고
 * @param transc user copy 처리를 위한 transc_t 객체
 * @param _pipe zero copy 처리를 위한 pipe_t 객체
 * @param fd 연결된 src file descriptor
 */
static int proxy_send_data( transc_t *transc, pipe_t *_pipe, int fd){
	if( fd < 0){
		printf("        | ! Proxy : Fail to get src_fd in proxy_send_data (fd:%d)\n", fd);
		return FD_ERR;
	}

	if( _pipe == NULL){
		printf("        | ! Proxy : Fail to get pipe in proxy_send_data (fd:%d)\n", fd);
		return BUF_ERR;
	}


	int write_bytes = 0;
	int splice_send_bytes = 0;

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
				printf("    | ! Proxy : unknown error, transc->send_bytes is negative in proxy_send_data (fd:%d)\n", fd);
				return UNKNOWN;
			}
		}
		else{
			printf("    | ! Proxy : transc->send_bytes error, not sended header, nevertheless send_bytes is not less than MSG_HEADER_LEN in proxy_send_data (fd:%d)\n", fd);
			return UNKNOWN;
		}

		if( ( write_bytes = write( fd, transc->write_hdr_buf, MSG_HEADER_LEN - transc->send_bytes, 0)) <= 0){
			printf("        | ! Proxy : Fail to write msg (fd:%d)\n", fd);
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
		printf("	| ! Proxy : send unknown error, not send hdr but send body? (in recv msg header) (fd:%d)\n", fd);
		return UNKNOWN;
	}

	// 2. Send body with <splice> function
	if( ( transc->is_send_header == 1) && ( transc->is_send_body == 0)){
		if( ( splice_send_bytes = splice( _pipe->fd[ 0], NULL, fd, NULL, transc->recv_bytes, SPLICE_F_NONBLOCK | SPLICE_F_MOVE)) < 0){
			if( errno == EAGAIN || errno == EWOULDBLOCK){
				return ERRNO_EAGAIN;
			}

			printf("        | ! Proxy : Fail to send msg (fd:%d)\n", fd);
			return NEGATIVE_BYTE;
		}
		else{
			transc->recv_bytes -= splice_send_bytes;
			transc->send_bytes += splice_send_bytes;

			if( transc->recv_bytes == 0){
				transc->is_send_body = 1;
			}
		}
	}

	if( ( transc->is_send_header == 1) && ( transc->is_send_body == 1)){
		//printf("        | @ Proxy : Send the msg (bytes : %d) (fd : %d)\n", transc->send_bytes, fd);
	}

	//printf("        | ! Proxy : Fail to send the msg (fd : %d)\n", fd);
	return NORMAL;
}

/**
 * @fn static void proxy_process_data( int fd, void *data)
 * @brief 연결된 fd 와 관련된 이벤트 발생 시 실행될 함수
 * @return void
 * @param fd 연결된 Source file descriptor
 * @param event 발생한 event
 */
static void proxy_process_data( int fd, void *data){
	int read_rv = NORMAL;
	int send_rv = NORMAL;
	int src_fd = ( int)( ( ( jpool_work_data_t*)( data))->src_fd);
	int dst_fd = ( int)( ( ( jpool_work_data_t*)( data))->dst_fd);
	pipe_t *_pipe;
	transc_t *transc;
	transc_t *ingoing_transc = ( transc_t*)( ( ( jpool_work_data_t*)( data))->ingoing_transc);
	transc_t *outgoing_transc = ( transc_t*)( ( ( jpool_work_data_t*)( data))->outgoing_transc);

	// fd 가 destination 이면 source 로 포워딩되로록 설정
	// 파이프는 dst->src 용 파이프로 설정
	if( fd == dst_fd){
		dst_fd = src_fd;
		_pipe = ( pipe_t*)( ( ( jpool_work_data_t*)( data))->_pipe2);
		transc = outgoing_transc;
	}
	else{
		_pipe = ( pipe_t*)( ( ( jpool_work_data_t*)( data))->_pipe1);
		transc = ingoing_transc;
	}

	// Forwarding ([src->dst] or [dst->src])
	// @ 순서 @
	// 1. 프록시에서 src 로 부터 요청 메시지 읽음
	// 2. 프록시에서 읽은 요청 메시지를 dst 로 포워딩함
	// 3. 프록시에서 dst 로 부터 응답 메시지 읽음
	// 4. 프록시에서 읽은 응답 메시지를 src 로 포워딩함

	if( ( transc->is_recv_header == 0) || ( transc->is_recv_body == 0)){
		read_rv = proxy_recv_data( transc, _pipe, fd);
		if( read_rv <= ZERO_BYTE){
			if( fd == src_fd){
				printf("	| ! Proxy : socket closed (fd:%d)\n\n", fd);
				close( fd);
			}
		}
	}

	if( ( transc->is_recv_header == 1) && ( transc->is_recv_body == 1)){
		if( ( send_rv = proxy_send_data( transc, _pipe, dst_fd)) <= ZERO_BYTE){
			printf("        | ! Proxy : Fail to send msg (fd:%d)\n", fd);
		}
	}

	if( ( transc->is_send_header == 1) && ( transc->is_send_body == 0)){
		if( ( send_rv = proxy_send_data( transc, _pipe, dst_fd)) <= ZERO_BYTE){
			printf("        | ! Proxy : Fail to send msg (fd:%d)\n", fd);
		}
	}

	if( ( transc->is_send_header == 1) && ( transc->is_send_body == 1)){
		transc_clear_data( transc);
	}
}

/**
 * @fn static int proxy_wait( proxy_t *proxy){
 * @brief Source fd 를 accept 하기 위한 함수
 * @return Source file descriptor
 * @param proxy 데이터 처리를 위한 Proxy 객체
 */
static int proxy_wait( proxy_t *proxy){
	if( proxy_check_fd( proxy->dst_fd) == FD_ERR){
		return SOC_ERR;
	}

	struct sockaddr_in src_addr;
	int proxy_fd = proxy->fd;
	int rv;
	int reuse = 1;
	int src_addr_len = sizeof( src_addr);
	memset( &src_addr, 0, src_addr_len);

	int src_fd = accept( proxy->fd, ( struct sockaddr*)( &src_addr), ( socklen_t*)( &src_addr_len));
	if( src_fd < 0){
		//printf("	| ! Proxy : accept error\n");
		return FD_ERR;
	}

	rv = proxy_set_fd_nonblock( src_fd);
	if( rv < 0){
		return rv;
	}

	// 만약 모든 쓰레드가 동작 중일 때 src 에서 연결 시도하면, 대기시킨다.
	if( proxy->jpool->working_cnt == THREAD_NUM){
		printf("	| @ Proxy : Waiting to connect with [fd:%d]\n", src_fd);
		while( 1){
			if( proxy->jpool->working_cnt < THREAD_NUM){
				break;
			}
			sleep( 1);
		}
	}

	dlist_int_add_node( proxy->fd_list, src_fd);
	printf("	| @ Proxy : fd_list length : %d\n", proxy->fd_list->length);

	printf("	| @ Proxy : Success to connect with src host (fd:%d)\n", src_fd);
	return src_fd;
}

/**
 * @fn static int proxy_open_dst( proxy_t *proxy){
 * @brief Destination 으로 connect 하기 위한 함수
 * @return Destination file descriptor
 * @param proxy 데이터 처리를 위한 Proxy 객체, Destination IP & PORT 정보 포함
 */
static int proxy_open_dst( proxy_t *proxy){
	char *dst_ip = proxy->dst_ip;
	int dst_port = proxy->dst_port;
	int addr_len, rv, reuse = 1;
	int dst_fd;
	struct sockaddr_in dst_addr;
	struct hostent *dst_host = gethostbyname( dst_ip);

	if( dst_host == NULL){
		printf("	| ! Proxy : gethostbyname error (struct hostent)\n");
		return HOST_ERR;
	}

	addr_len = sizeof( dst_addr);

	if( ( dst_fd = socket( AF_INET, SOCK_STREAM, 0)) < 0){
		printf("	| ! Proxy : Fail to open dst socket\n");
		return errno;
	}

	if( setsockopt( dst_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse))){
		printf("	| ! Proxy : Fail to set the dst socket's option\n");
		return errno;
	}

	memset( &dst_addr, 0, addr_len);
	dst_addr.sin_family = AF_INET;
	memcpy( &dst_addr.sin_addr, dst_host->h_addr, dst_host->h_length);
	dst_addr.sin_port = htons( dst_port);

	rv = proxy_set_fd_nonblock( dst_fd);
	if( rv < 0){
		return rv;
	}

	while( 1){
		sleep( 1);
		rv = connect( dst_fd, ( struct sockaddr*)( &dst_addr), addr_len);
		if( rv == -1){
			if( errno == EINPROGRESS){
				//printf("# EINPROGRESS\n");
				continue;
			}
			else{
				//printf("# ERRNO : %d\n", errno);
			}
		}
		else{
			break;
		}
	}

	return dst_fd;
}

/**
 * @fn static int proxy_set_sock( proxy_t *proxy);
 * @brief Source 와 연결될 Proxy 의 socket fd 를 설정한다.
 * @return 정상 연결되면 NORMAL, 비정상 연결이면 FD_ERR 또는 SOC_ERR 반환
 * @param proxy fd 를 설정할 Proxy 객체, local IP & PORT 정포 포함
 */
static int proxy_set_sock( proxy_t *proxy){
	char *local_ip = proxy->local_ip;
	int local_port = proxy->local_port;
	int addr_len, rv, reuse = 1;

	if( ( proxy->fd = socket( AF_INET, SOCK_STREAM, 0)) < 0){
		printf("	| ! Proxy : Fail to open proxy socket\n");
		free( proxy);
		return FD_ERR;
	}

	if( setsockopt( proxy->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse))){
		printf("	| ! Proxy : Fail to set the proxy socket's option\n");
		free( proxy);
		return FD_ERR;
	}

	addr_len = sizeof( proxy->addr);
	memset( &proxy->addr, 0, addr_len);
	proxy->addr.sin_family = AF_INET;
	proxy->addr.sin_addr.s_addr = inet_addr( local_ip);
	proxy->addr.sin_port = htons( local_port);

	if( bind( proxy->fd, ( struct sockaddr*)( &proxy->addr), addr_len) < 0){
		printf("	| ! Proxy : Fail to bind proxy socket\n");
		return SOC_ERR;
	}

	if( listen( proxy->fd, MSG_QUEUE_NUM)){
		printf("	| ! Proxy : listen error\n");
		return SOC_ERR;
	}

	rv = proxy_set_fd_nonblock( proxy->fd);
	if( rv < 0){
		return rv;
	}

	return NORMAL; 
}

/**
 * @fn static int proxy_delete_src_info( proxy_t *proxy, int src_fd)
 * @brief 등록된 Source fd 와 관련된 정보를 Proxy 관리 리스트에서 삭제하기 위한 함수
 * @return fd 관련 데이터 삭제 성공 여부
 * @param proxy Proxy 관리 리스트 정보를 가져오기 위한 Proxy 객체
 * @param src_fd 삭제하려는 Source fd
 */
static int proxy_delete_src_info( proxy_t *proxy, int src_fd){
	int rv;

	rv = dlist_int_del_node_by_data( proxy->fd_list, src_fd);
	if( rv < NORMAL){
		printf("	| ! Proxy : Fail to delete the source fd(%d) in proxy fd_list\n", src_fd);
		return rv;
	}
	else{
		printf("	| @ Proxy : fd_list length : %d\n", proxy->fd_list->length);
		printf("	| @ Proxy : Success to delete the source fd(%d) in proxy fd_list\n", src_fd);
	}

	jpool_work_data_t temp_work_data[ 1];
	jpool_set_work_data( temp_work_data, src_fd, proxy->dst_fd, NULL);
	jpool_work_data_t *work_data = dlist_ptr_find_node_data_by_data( proxy->work_list, temp_work_data);
	if( ( work_data != NULL) && ( work_data->src_fd == src_fd)){
		// Delete the node & node data(work_data)
		dlist_ptr_del_node_by_data( proxy->work_list, work_data);
		printf("	| @ Proxy : work_list length : %d\n", proxy->work_list->length);
		printf("	| @ Proxy : Success to delete the work data related with source fd(%d) in proxy work_list\n", src_fd);
	}
	else{
		printf("	| ! Proxy : Fail to delete the work data related with source fd(%d) in proxy fd_list\n", src_fd);
		return NOT_EXIST;
	}

//	jpool_destroy_work_data( work_data);
	printf("\n");
	return NORMAL;
}

/**
 * @fn static void proxy_conn( proxy_t *proxy)
 * @brief Source 와 연결되었을 때 데이터를 수신하고 데이터를 처리하기 위한 함수
 * @return void
 * @param proxy 데이터 처리를 위한 Proxy 객체
 */
static void proxy_conn( void *data){
	int rv;
	int src_fd = ( int)( ( ( jpool_work_data_t*)( data))->src_fd);
	int dst_fd = ( int)( ( ( jpool_work_data_t*)( data))->dst_fd);

	proxy_t *proxy = ( proxy_t*)( ( ( jpool_work_data_t*)( data))->arg);

	transc_t *ingoing_transc = ( transc_t*)( ( ( jpool_work_data_t*)( data))->ingoing_transc);
	transc_clear_data( ingoing_transc);
	transc_t *outgoing_transc = ( transc_t*)( ( ( jpool_work_data_t*)( data))->outgoing_transc);
	transc_clear_data( outgoing_transc);

	pipe_t *_pipe1 = ( pipe_t*)( ( ( jpool_work_data_t*)( data))->_pipe1);
	rv = pipe_open( _pipe1);
	if( rv < 0){
		printf("	| ! Proxy : pipe1(src->dst) open error in the connection [src:%d] with [dst:%d]\n", src_fd, dst_fd);
		return ;
	}

	pipe_t *_pipe2 = ( pipe_t*)( ( ( jpool_work_data_t*)( data))->_pipe2);
	rv = pipe_open( _pipe2);
	if( rv < 0){
		printf("	| ! Proxy : pipe2(dst->src) open error in the connection [src:%d] with [dst:%d]\n", src_fd, dst_fd);
		return ;
	}

	int i, event_count = 0;
	struct epoll_event src_event;
	src_event.events = EPOLLIN;
	src_event.data.fd = src_fd;
	if( ( epoll_ctl( proxy->epoll_handle_fd, EPOLL_CTL_ADD, src_fd, &src_event)) < 0){
		printf("    | ! Proxy : Fail to add epoll src_event\n");
		return ;
	}

	while( 1){
		if( ( proxy_check_fd( src_fd) == FD_ERR) || ( proxy_check_fd( dst_fd) == FD_ERR)){
			printf("	| @ Proxy : connection is ended (%d <-> %d)\n", src_fd, dst_fd);
			rv = proxy_delete_src_info( proxy, src_fd);
			if( rv < NORMAL){
				printf("	| ! Proxy : Fail to delete the source fd(%d) data\n", src_fd);
			}
			else{
				printf("	| @ Proxy : Success to delete the source fd(%d) data\n", src_fd);
			}
			break;
		}

		//printf("        | @ Proxy : waiting...\n");
		event_count = epoll_wait( proxy->epoll_handle_fd, proxy->events, BUF_MAX_LEN, TIMEOUT);
		if( event_count < 0){
			printf("    | ! Proxy : epoll_wait error\n");
			break;
		}
		else if( event_count == 0){
			printf("    | ! Proxy : epoll_wait timeout\n");
			continue;
		}

		for( i = 0; i < event_count; i++){
			if( proxy->events[ i].data.fd == src_fd){
				proxy_process_data( src_fd, data);
			}
			else if( proxy->events[ i].data.fd == dst_fd){
				proxy_process_data( dst_fd, data);
			}
		}
	}

	pipe_close( _pipe1);
	pipe_close( _pipe2);
}

/**
 * @fn static int proxy_add_work_data( proxy_t *proxy)
 * @brief Proxy 에서 Source fd 에 관련된 작업을 위한 데이터를 추가하기 위한 함수
 * @return 데이터 추가 성공 여부 반환
 * @param proxy 현재 사용되는(할당된) Proxy 객체
 */
static int proxy_add_work_data( proxy_t *proxy){
	int src_fd = dlist_int_find_last_node_data( proxy->fd_list);

	jpool_work_data_t *work_data = jpool_create_work_data( src_fd, proxy->dst_fd, proxy);
	dlist_ptr_add_node( proxy->work_list, ( void*)( work_data));
	printf("	| @ Proxy : work_list length : %d\n", proxy->work_list->length);

	if( jpool_add_work( proxy->jpool, proxy_conn, ( jpool_work_data_t*)( dlist_ptr_find_last_node_data( proxy->work_list))) == false){
		printf("	| ! Proxy : Fail to add work\n");
		return OBJECT_ERR;
	}
	return NORMAL;
}

/**
 * @fn static void* proxy_detect_finish( void *data)
 * @brief Proxy 에서 사용하는 메모리를 해제하기 위한 함수
 * @return None
 * @param data Thread 매개변수, 현재 사용되는(할당된) Proxy 객체
 */
static void* proxy_detect_finish( void *data){
	proxy_t *proxy = ( proxy_t*)( data);

	while( 1){
		//if( proxy_check_fd( proxy->dst_fd) == FD_ERR){
		//	proxy_destroy( proxy);
		//	break;
		//}

		if( is_error == true){
			break;
		}

		if( is_finish == true){
			proxy_destroy( proxy);
			is_error = true;
			break;
		}
		
		sleep( 1);
	}
}

/**
 * @fn static int proxy_handle_finish( proxy_t *proxy)
 * @brief Proxy 가 종료되기 위해 종료 여부를 확인하는 Thread 구동하기 위한 함수
 * @return Thread 생성 시 오류 발생 여부 반환
 * @param proxy 현재 사용되는(할당된) Proxy 객체
 */
static int proxy_handle_finish( proxy_t *proxy){
	pthread_t thread;

	if( ( pthread_create( &thread, NULL, proxy_detect_finish, proxy)) != 0){
		printf("	| ! Proxy : Fail to create a thread for detecting is_finish var\n");
		return PTHREAD_ERR;
	}

	if( ( pthread_detach( thread)) != 0){
		printf("	| ! Proxy : Fail to detach the thread for detecting is_finish var\n");
		return PTHREAD_ERR;
	}

	return NORMAL;
}

// -----------------------------------------

/**
 * @fn proxy_t* proxy_init()
 * @brief Proxy 객체를 생성하고 초기화하는 함수
 * @return 생성된 Proxy 객체
 */
proxy_t* proxy_init( char **argv){
	proxy_t *proxy = ( proxy_t*)malloc( sizeof( proxy_t));

	if ( proxy == NULL){
		printf("	| ! Proxy : Fail to allocate memory\n");
		return NULL;
	}

	proxy_set_signal( SIGINT);
	if( ( proxy_handle_finish( proxy)) == PTHREAD_ERR){
		free( proxy);
		return NULL;
	}

	proxy->local_ip = strdup( argv[1]);
	proxy->local_port = atoi( argv[2]);
	proxy->dst_ip = strdup( argv[3]);
	proxy->dst_port = atoi( argv[4]);

	dlist_int_init( proxy->fd_list);
	dlist_ptr_init( proxy->work_list);

	int rv = proxy_set_sock( proxy);
	if( rv == FD_ERR){
		free( proxy);
		return NULL;
	}
	else if( rv == SOC_ERR){
		close( proxy->fd);
		free( proxy);
		return NULL;
	}

	proxy->jpool = jpool_init( THREAD_NUM);
	if( proxy->jpool == NULL){
		close( proxy->fd);
		free( proxy);
		return NULL;
	}

	proxy->dst_fd = proxy_open_dst( proxy);
	if( proxy->dst_fd <= FD_ERR){
		printf("	| ! Proxy : dst host fd error (%d)\n", proxy->dst_fd);
		close ( proxy->fd);
		free( proxy);
		return NULL;
	}
	else{
		printf("	| @ Proxy : Success to connect with dst host (fd:%d)\n", proxy->dst_fd);

		if( ( proxy->epoll_handle_fd = epoll_create( BUF_MAX_LEN)) < 0){
			printf("    | ! Proxy : Fail to create epoll handle fd\n");
			return ;
		}

		struct epoll_event dst_event;
		dst_event.events = EPOLLIN;
		dst_event.data.fd = proxy->dst_fd;
		if( ( epoll_ctl( proxy->epoll_handle_fd, EPOLL_CTL_ADD, proxy->dst_fd, &dst_event)) < 0){
			printf("    | ! Proxy : Fail to add epoll dst_event\n");
			return ;
		}
	}


	printf("	| @ Proxy : Success to create a object\n");
	printf("	| @ Proxy : Welcome\n\n");
	return proxy;
}

/**
 * @fn void proxy_destroy( proxy_t *proxy)
 * @brief Proxy 객체를 삭제하기 위한 함수
 * @return void
 * @param proxy 삭제하기 위한 Proxy 객체
 */
void proxy_destroy( proxy_t *proxy){
	if( proxy->local_ip){
		free( proxy->local_ip);
	}

	if( proxy->dst_ip){
		free( proxy->dst_ip);
	}

	if( proxy_check_fd( proxy->fd) != FD_ERR){
		close( proxy->fd);
	}

	if( proxy_check_fd( proxy->dst_fd) != FD_ERR){
		close( proxy->dst_fd);
	}

	dlist_int_destroy( proxy->fd_list);
	dlist_ptr_destroy( proxy->work_list);

	jpool_destroy( proxy->jpool);
	free( proxy);
	printf("	| @ Proxy : Success to destroy the object\n");
	printf("	| @ Proxy : BYE\n\n");
}

/**
 * @fn void proxy_handle_req( proxy_t *proxy)
 * @brief Proxy 로 수신되는 요청을 처리하기 위한 함수
 * @return void
 * @param proxy 데이터 처리를 위한 Proxy 객체
 */
void proxy_handle_req( proxy_t *proxy){
	int rv;
	int working_cnt;

	while( 1){
		//working_cnt = proxy->jpool->working_cnt;
		//printf("\n        | @ Proxy : waiting... (using:%d / wait:%d)\n", working_cnt, THREAD_NUM - working_cnt);

		rv = proxy_wait( proxy);
		if( rv <= FD_ERR){
			if( rv == SOC_ERR){
				printf("	| ! Proxy : destination fd closed\n");
				break;
			}
			continue;
		}

		rv = proxy_add_work_data( proxy);
		if( rv < NORMAL){
			break;
		}
	}
}

// -----------------------------------------

/**
 * @fn int main( int argc, char **argv)
 * @brief Proxy 구동을 위한 main 함수
 * @return int
 * @param argc 매개변수 개수
 * @param argv ip 와 포트 정보
 */
int main( int argc, char **argv){
	if( argc != 5){
		printf("	| ! need param (total 4) : [local ip] [local port] [host ip] [host port]\n");
		return -1;
	}

	proxy_t *proxy = proxy_init( argv);
	if( proxy == NULL){
		printf("	| ! Proxy : Fail to initialize\n");
		return -1;
	}
	proxy_handle_req( proxy);
	if( is_finish == false){
		printf("	| @ Proxy : Please press <ctrl+c> to exit\n");
		is_error = true;
		while( 1){
			if( is_finish == true){
				break;
			}
			sleep( 1);
		}
		proxy_destroy( proxy);
	}
	else{
		while( 1){
			if( is_error == true){
				break;
			}
			sleep( 1);
		}
	}
}

