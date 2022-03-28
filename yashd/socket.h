#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3826
#define MAXCONN 20
#define MAXMSG 300

typedef struct Connection{
    int connfd;
    struct sockaddr_in connaddr;
} Connection;
// extern struct Connection *connections[MAXCONN];

Connection *init_connection();
void free_connection(Connection *conn);
int create_server_socket(FILE *log);
int create_client_socket(const char *addr);
int write_socket(int sockfd, char *msg, FILE *log);

#endif