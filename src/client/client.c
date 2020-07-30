#include "client.h"

static int is_finish = false;
static int is_error = false;

// -----------------------------------------

static void client_signal_handler( int sig){
	printf("	| ! Client : client will be finished (sig:%d)\n", sig);
	is_finish = true;
	signal( sig, SIG_DFL);
}

static void client_set_signal( int sig_type){
	signal( sig_type, client_signal_handler);
}

/**
 * @fn static void* client_detect_finish( void *data)
 * @brief Client 에서 사용하는 메모리를 해제하기 위한 함수
 * @return None
 * @param data Thread 매개변수, 현재 사용되는(할당된) Client 객체
 */
static void* client_detect_finish( void *data){
	client_t *client = ( client_t*)( data);

	while( 1){
		//if( client_check_fd( client->dst_fd) == FD_ERR){
		//	client_destroy( client);
		//	break;
		//}

		if( is_error == true){
			break;
		}

		if( is_finish == true){
			client_destroy( client);
			is_error = true;
			break;
		}
	}
}

/**
 * @fn static int client_handle_finish( client_t *client)
 * @brief Client 가 종료되기 위해 종료 여부를 확인하는 Thread 구동하기 위한 함수
 * @return Thread 생성 시 오류 발생 여부 반환
 * @param client 현재 사용되는(할당된) Client 객체
 */
static int client_handle_finish( client_t *client){
	pthread_t thread;

	if( ( pthread_create( &thread, NULL, client_detect_finish, client)) != 0){
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
 * @fn client_t* client_init()
 * @brief server 객체를 생성하고 초기화하는 함수
 * @return 생성된 server 객체
 */
client_t* client_init( char **argv){
	int rv;
	client_t *client = ( client_t*)( malloc( sizeof( client_t)));

	if( client == NULL){
		printf("        | ! Client : Fail to allocate memory\n");
		return NULL;
	}

	if( ( client->fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		printf("        | ! Client : Fail to open socket\n");
		close( client->fd);
		free( client);
		return NULL;
	}

	int addr_len, reuse = 1;
	int server_fd;
	struct sockaddr_in server_addr;
	struct hostent *server_host = gethostbyname( argv[1]);

	if( server_host == NULL){
		printf("	| ! Client : gethostbyname error (struct hostent)\n");
		close( client->fd);
		free( client);
		return NULL;
	}

	addr_len = sizeof( server_addr);
	memset( &server_addr, 0, addr_len);
	server_addr.sin_family = AF_INET;
	memcpy( &server_addr.sin_addr, server_host->h_addr, server_host->h_length);
	server_addr.sin_port = htons( atoi( argv[2]));

	if( connect( client->fd, ( struct sockaddr*)( &server_addr), sizeof( struct sockaddr))){
		printf("        | ! Client : Fail to connect with Server\n");
		close( client->fd);
		free( client);
		return NULL;
	}

	printf("        | @ Client : Success to create a object & connect with Server\n");
	printf("        | @ Client : Welcome\n\n");
	return client;
}

/**
 * @fn void client_destroy( client_t *client)
 * @brief client 객체를 삭제하기 위한 함수
 * @return void
 * @param client 삭제하기 위한 client 객체
 */
void client_destroy( client_t *client){
	close( client->fd);
	free( client);

	printf("        | @ Client : Success to destroy the object\n");
	printf("        | @ Client : BYE\n\n");
}

/**
 * @fn void client_process_data( client_t *client)
 * @brief server 로 요청을 보내서 응답을 받는 함수
 * @return void
 * @param client 요청을 하기 위한 client 객체
 */
void client_process_data( client_t *client){
	int server_addr_size = 0;
	char read_buf[ BUF_MAX_LEN];
	char send_buf[ BUF_MAX_LEN];
	ssize_t recv_bytes, send_bytes;
	char msg[ BUF_MAX_LEN];

	while(1){
		memset( msg, '\0', BUF_MAX_LEN);
		printf("        | @ Client : > ");
		if( fgets( msg, BUF_MAX_LEN, stdin) == NULL){
			if( errno == EINTR){
				is_finish = true;
				break;
			}
		}

		msg[ strlen( msg) - 1] = '\0';
		snprintf( send_buf, BUF_MAX_LEN, "%s", msg);

		jmp_t send_msg[ 1];
		jmp_set_msg( send_msg, 1, send_buf, 1);
		jmp_print_msg( send_msg);
		if( ( send_bytes = write( client->fd, send_msg, jmp_get_msg_length( send_msg))) <= 0){
			printf("        | ! Client : Fail to send msg (bytes:%d) (errno:%d)\n", send_bytes, errno);
			break;
		}
		else{
			printf("        | @ Client : Success to send msg to Server (bytes:%d)\n", send_bytes);

//			if( !memcmp( send_buf, "q", 1)){
//				printf("        | @ Client : Finish\n");
//				break;
//			}

			printf("        | @ Client : Wait to recv msg...\n");

			jmp_t recv_msg[ 1];
			if( ( recv_bytes = read( client->fd, recv_msg, BUF_MAX_LEN)) <= 0){
				printf("        | ! Client : Fail to recv msg (bytes:%d) (errno:%d)\n\n", recv_bytes, errno);
				break;
			}
			else{
				jmp_print_msg( recv_msg);
				snprintf( read_buf, BUF_MAX_LEN, "%s", jmp_get_data( recv_msg)); 
				read_buf[ recv_bytes] = '\0';
				printf("        | @ Client : < %s (%lu bytes)\n", read_buf, recv_bytes);
			}
		}
		printf("\n");
	}
}


// -----------------------------------------

/**
 * @fn int main( int argc, char **argv)
 * @brief client 구동을 위한 main 함수
 * @return int
 * @param argc 매개변수 개수
 * @param argv ip 와 포트 정보
 */
int main( int argc, char **argv){
	if( argc != 3){
		printf("	| ! need param : server_ip server_port\n");
		return -1;
	}

	client_t *client = client_init( argv);
	if( client == NULL){
		printf("    | ! Client : Fail to initialize\n");
		return -1;
	}

	client_process_data( client);

	if( is_finish == false){
		printf("	| @ Server : Please press <ctrl+c> to exit\n");
		is_error = true;
		while( 1){
			if( is_finish == true){
				break;
			}
		}
		client_destroy( client);
	}
	else{
		while( 1){
			if( is_error == true){
				break;
			}
		}
	}
}


