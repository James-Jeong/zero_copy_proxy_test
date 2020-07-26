#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "jmp.h"

#define BUF_MAX_LEN 1024
#define SERVER_PORT 8000

static const char *server_ip = "127.0.0.1";

/// @struct client_t
/// @brief server 로 요청을 보내서 응답을 받기위한 구조체
typedef struct client_s client_t;
struct client_s{
	/// client tcp socket file descriptor
	int fd;
	/// server socket address
	struct sockaddr_in server_addr;
	/// client socket address
	struct sockaddr_in my_addr;
};

client_t* client_init( char* hostname);
void client_process_data( client_t *client);
void client_destroy( client_t *client);

