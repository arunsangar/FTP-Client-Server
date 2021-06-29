#ifndef TCP
#define TCP

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/sendfile.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h>

#define MAX_MSG_SIZE 100
#define MAX_CMD_MSG_SIZE 106
#define MAX_CMD_SIZE 5
#define MAX_FILE_NAME_SIZE 101

int tcp_send(const int& socket, const void* buffer, const int& buffSize);

int tcp_recv(const int& socket, void* buffer, const int& buffSize);

#endif