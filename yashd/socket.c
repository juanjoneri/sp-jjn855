#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "socket.h"

Connection *init_connection(){
    Connection *conn = malloc(sizeof(*conn) );
    return conn;
}

void free_connection(Connection *conn){
    free(conn);
}

// Code form Dr Y lecture on sockets
void reuse_port(int s) {
    int one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*) &one, sizeof(one)) == -1) {
        printf("Error in setsockopt, SO_REUSEPORT\n");
        exit(EXIT_FAILURE);
    }
}


/* Create and Initialize Socket
1. Create: socket()
2. Bound Attributes: bind()
3. Listen: listen()
*/
int create_server_socket(FILE *logfd){
    int sockfd;
    struct sockaddr_in servaddr;

    //---------------------------------------------------- 1. Create socket ----------------------------------------------------
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        exit(EXIT_FAILURE);
    }

    // 2. Bind attributes to socket
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    reuse_port(sockfd);
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        exit(EXIT_FAILURE);
    }


    //---------------------------------------------------- 3. Listen for connections ----------------------------------------------------
    if(listen(sockfd, MAXCONN) == -1){
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

/* Create and Initialize Socket
 * 1. Create: socket()
 * 2. Connect to server: connect()
 */
int create_client_socket(const char *addr) {
    int sockfd;
    struct sockaddr_in servaddr;

    //---------------------------------------------------- 1. Create socket ----------------------------------------------------
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        // printf("Error creating socket\n");
        exit(EXIT_FAILURE);
    }
    // printf("Socket created successfully\n");

    // 2. Bind attributes to socket
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(PORT);

    // printf("    (TCP/Client INET ADDRESS is: %s )\n", inet_ntoa(server.sin_addr));
    printf("    (TCP/Server INET ADDRESS is: %s )\n", inet_ntoa(servaddr.sin_addr));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("Error connecting to socket\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to TCP Server: ");
    printf("%s:%d\n", inet_ntoa(servaddr.sin_addr),
	   ntohs(servaddr.sin_port));

    return sockfd;
}


int write_socket(int sockfd, char *msg, FILE *logfd){
    if(send(sockfd, msg, strlen(msg), 0) == -1){
        return -1;
    }
    return 0;
}