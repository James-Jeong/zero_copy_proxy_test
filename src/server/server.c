#include "server.h"

/**
 * @mainpage Project : zero copy proxy test
 * @section intro 소개
 *    - 차후 계획한 개인 프로젝트를 위해 미리 제작 
 * @section  CreateInfo 작성 정보
 *    - 작성자 :   정동욱
 *    - 작성일 :   2020/07/10
 * @subsection exec 실행 방법 및 인수 설명
 *    - 실행 방법\n
 *      서버       : SERVER/server
 *      클라이언트 : CLIENT/client <hostname>
 */

/**
 * @fn int main( int argc, char **argv)
 * @brief server 구동을 위한 main 함수
 * @return int
 * @param argc 매개변수 개수
 * @param argv ip 와 포트 정보
 */
int main( int argc, char **argv){
	//	if( argc != 3){
	//		printf("	| ! need param : ip port\n");
	//		return -1;
	//	}

	server_t *server = server_init();
	if( server == NULL){
		printf("	| ! Server : Fail to initialize\n");
		return -1;
	}

	server_conn( server);

	server_destroy( server);
}



// -----------------------------------------

/**
 * @fn server_t* server_init()
 * @brief server 객체를 생성하고 초기화하는 함수
 * @return 생성된 server 객체
 */
server_t* server_init(){
	server_t *server = ( server_t*)malloc( sizeof( server_t));

	if ( server == NULL){
		printf("	| ! Server : Fail to allocate memory\n");
		return NULL;
	}


	memset( &server->addr, 0, sizeof( struct sockaddr));
	server->addr.sin_family = AF_INET;
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server->addr.sin_port = htons(SERVER_PORT);

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
		return;
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
 * @fn void server_conn( server_t *server)
 * @brief client 와 연결되었을 때 데이터를 수신하고 데이터를 처리하기 위한 함수
 * @return void
 * @param server 데이터 처리를 위한 server 객체
 */
void server_conn( server_t *server){
	if( server->fd <= 0){
		printf("	| ! Server : fd error\n");
		return;
	}

	int rv = 0;
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof( client_addr);
	memset( &client_addr, 0, client_addr_len);

	int client_fd = accept( server->fd, ( struct sockaddr*)( &client_addr), ( socklen_t*)( &client_addr_len));
	if( client_fd < 0){
		printf("	| ! Server : accept error\n");
		return;
	}

	int flag = fcntl( client_fd, F_GETFL, 0);
	fcntl( client_fd, F_SETFL, flag | O_NONBLOCK);

	rv = server_process_data( server, client_fd);
	if( rv < 0){
		printf("	| ! Server : process end\n");
		return ; 
	}
}

int server_process_data( server_t *server, int fd){
	bool read_rv = false;
	bool send_rv = false;
	buf_t buffer[ 1];
	if( ( server_buf_data_init( buffer)) == false){
		return -2;
	}

	printf("        | @ Server : waiting...\n");
	while( 1){
		sleep( 1);
		server_buf_clear( buffer);

		read_rv = server_recv_data( server, fd, buffer);
		if( read_rv == false){
			printf("	| ! Server : disconnected\n");
			printf("	| ! Server : socket close\n");
			close( fd);
			return -1;
		}
		else{
			if( ( send_rv = server_send_data( server, fd, buffer)) == false){
				printf("        | ! Server : Fail to send msg\n");
			}
		}
	}
	return 1;
}

bool server_recv_data( server_t *server, int fd, void *data){
	if( fd < 0){
		printf("        | ! Server : Fail to get fd in server_recv_data\n");
		return false;
	}

	buf_t *buffer = ( buf_t*)( data);
	int recv_bytes = 0;
	jmp_t recv_msg;

	while( 1){
		sleep( 1);
		if( ( recv_bytes = read( server->fd, &recv_msg, BUF_MAX_LEN - buffer->bytes)) <= 0){
			if( errno == EAGAIN || errno == EWOULDBLOCK){
				continue;
			}

			printf("    | ! Server : Fail to recv msg (bytes:%d) (errno:%d)\n\n", recv_bytes, errno);
			return false;
		}
		else{
			printf("        | @ Server : Success to recv msg from client (bytes:%d)\n", recv_bytes);
			jmp_print_msg( &recv_msg);
			buffer->bytes += recv_bytes;
			if( buffer->bytes >= HDR_MAX_LEN){
				break;
			}
		}
	}

	return true;
}

bool server_send_data( server_t *server, int fd, void *data){
	if( fd < 0){
		printf("        | ! Server : Fail to get fd in server_recv_data\n");
		return false;
	}

	buf_t *buffer = ( buf_t*)( data);
	int buf_bytes = 0;
	int send_bytes = 0;

	jmp_t send_msg;
	jmp_set_msg( &send_msg, 1, buffer->data, 2);
	jmp_print_msg( &send_msg);
	if( ( send_bytes = write( server->fd, &send_msg, jmp_get_msg_length( &send_msg), 0)) <= 0){
		printf("        | ! Server : Fail to send msg (bytes:%d) (errno:%d)\n", send_bytes, errno);
		return false;
	}
	else{
		printf("        | @ Server : Success to send msg to client (bytes:%d)\n", send_bytes);
		buffer->bytes -= send_bytes;
	}

	return true;
}

bool server_buf_data_init( buf_t *buffer){
	buffer->data = ( char*)malloc( BUF_MAX_LEN);
	if( buffer->data == NULL){
		return false;
	}
	return true;
}

void server_buf_destroy( buf_t *buffer){
	if( buffer->data){
		free( buffer->data);
	}
}

void server_buf_clear( buf_t *buffer){
	if( buffer->data){
		memset( buffer->data, '\0', BUF_MAX_LEN);
	}
	buffer->bytes = 0;
}

