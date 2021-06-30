#include "helper.h"

void console(int connfd, char* ipAddr);
int  initConn(char* ipAddr, int port);
void send_file(int connfd, char* ipAddr, char* filename);
void recv_file(int connfd, char* ipAddr, char* filename);
void recv_ls(int connfd);
  
int main(int argc, char** argv) {
	//port number and socket file descriptor
	int port = -1;
	int connfd = -1;
	
	//verify command line parameters
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <SERVER IP> <SERVER PORT #>", argv[0]);
		exit(-1);	
	}
	
	//convert port number to integer
	port = atoi(argv[2]);
	
	//verify port number is within range
	if(port < 0 || port > 65535) {
		fprintf(stderr, "Error: invalid port number\n");
		exit(-1);
	}
	
	//connect to server
	connfd = initConn(argv[1], port);
	
	//console loop for ftp client
	console(connfd, argv[1]);
	
	//close control connection
	close(connfd);
    
    return 0; 
}

void console(int connfd, char* ipAddr) {
	//variables for client side parsing of user input
	bool openSession = true;
	
	//cmd: command string
	//filename: file name string
	//cmdMsg: command message string (command + filename)
	char cmd[MAX_CMD_SIZE];
	char filename[MAX_FILE_NAME_SIZE];
	char cmdMsg[MAX_CMD_MSG_SIZE];
	
	//extraChars: used for counting extra characters in command message
	int  extraChars = 0;
	
	//tempStr: temporary message string
	char tempStr[MAX_MSG_SIZE];
	
	//get confirmation message from server
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	fprintf(stderr, "%s", tempStr);
	
	//console loop: open until user enters quit command
	while(openSession) {
		//console prompt
		fprintf(stderr, "FTP> ");
		
		//get the command from the user to send to the server
		scanf("%4s", cmd);
		
		//get a file name if it is a get/put command
		if(strcmp(cmd, "get") == 0 || strcmp(cmd, "put") == 0)
			scanf("%100s", filename);
		
		//clear input buffer
		while ((getchar()) != '\n') extraChars++;
		
		//validate command - get/put/ls/quit
		if((strcmp(cmd, "get") == 0 || strcmp(cmd, "put") == 0
			|| strcmp(cmd, "ls") == 0 || strcmp(cmd, "quit") == 0) && extraChars == 0) {
			
			//command message format - cmd|filename|
			strcpy(cmdMsg, cmd);
			strcat(cmdMsg, "|");
			strcat(cmdMsg, filename);
			strcat(cmdMsg, "|");
			
			//send command message
			if(tcp_send(connfd, cmdMsg, MAX_CMD_MSG_SIZE) < 0) {
				perror("Error tcp_send");
				exit(-1);
			}
			
			//put command - send file to server
			if(strcmp(cmd, "put") == 0) 
				send_file(connfd, ipAddr,  filename);
			
			//get command - receive file from server
			else if(strcmp(cmd, "get") == 0)
				recv_file(connfd, ipAddr, filename);
			
			else if(strcmp(cmd, "ls") == 0)
				recv_ls(connfd);
			
			//get response message and print to console
			if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
				perror("Error tcp_recv");
				exit(-1);
			}
			fprintf(stderr, "%s", tempStr);
			
			//quit command - break loop if user is done
			if(strcmp(cmd, "quit") == 0) {
				openSession = false;
				if(close(connfd) < 0) {
					//print error message
					perror("Error close connection");
					exit(-1);
				}
			}
		}
		//print error message
		else {
			fprintf(stderr, "Error: invalid command\n");
			extraChars = 0;
		}
	}
}

int initConn(char* ipAddr, int port) {
	
	//connfd: connection file descriptor
	int connfd = -1;
	
	//server address struct
	sockaddr_in serverAddr;
	
	//create socket to connect to server (TCP - reliable)
	if((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error socket");
		exit(-1);
	}
	
	//initialize server address struct
	memset(&serverAddr, 0, sizeof(serverAddr));
		
	//set server family
	serverAddr.sin_family = AF_INET;
	
	//convert port number to network byte order
	serverAddr.sin_port = htons(port);
	
	//convert IP address
	if(!inet_pton(AF_INET, ipAddr, &serverAddr.sin_addr)) {
		perror("Error inet_pton");
		exit(-1);
	}
	
	//connect to server's control port
	if(connect(connfd, (sockaddr*)&serverAddr, sizeof(sockaddr)) < 0) {
		perror("Error connect");
		exit(-1);
	}
	
	//return established connection file descriptor
	return connfd;
}

void send_file(int connfd, char* ipAddr, char* filename) {
	
	//tempStr: temporary message string
	char tempStr[MAX_MSG_SIZE];
	
	//fileSize: size of file
	//port: ephemeral port number
	//datafd: data transfer connection socket
	int fileSize = 0;
	int port = -1;
	int datafd = -1;
	
	//fd: open file descriptor
	int fd = open(filename, O_RDONLY);
	
	//verify file was opened
	if(fd < 0) {
		perror("Error fopen");
		return;
	}	
	
	//get file size
	fileSize = getFileSize(filename);
	
	//convert file size to string format and save to tempStr
	if(snprintf(tempStr, MAX_MSG_SIZE, "%d", fileSize) < 0) {
		perror("Error snprintf");
		exit(-1);
	}
	
	//send file size to server
	if(tcp_send(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	//recieve port number for data transfer connection
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	
	//convert port number to int
	port = atoi(tempStr);
	
	//initialize data transfer connection
	datafd = initConn(ipAddr, port);
	
	//get connection confirmation message from server
	if(tcp_recv(datafd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	fprintf(stderr, "%s", tempStr);
	
	//numSent: number of bytes sent in the last sendfile()
	//totalNumSent: total number of bytes sent of the file
	int numSent = 0;
	off_t totalNumSent = 0;
	
	//print sending message	
	fprintf(stderr, "Sending file: %s (%d bytes)...\n", filename, fileSize);	
	
	//send while number of bytes sent has not exceeded file size
	while(totalNumSent < fileSize)
	{	
		//send bytes to server
		if((numSent = sendfile(datafd, fd, &totalNumSent, fileSize - totalNumSent)) < 0) {
			perror("Error sendfile");
			exit(-1);
		}
		
		//update total number of bytes sent
		totalNumSent += numSent;
				
	}

	//close data connection
	close(datafd);
	
	//close file descriptor
	close(fd);
}

void recv_file(int connfd, char* ipAddr, char* filename) {
	
	//tempStr: temporary message string
	char tempStr[MAX_MSG_SIZE];
	
	//fileSize: size of file
	//port: ephemeral port number
	//datafd: data transfer connection socket
	int fileSize = 0;
	int port = -1;
	int datafd = -1;
	
	//receive file size from server
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	
	//convert file size from string to integer
	fileSize = atoi(tempStr);
		
	//file pointer for write file
	FILE *fp;
	
	//open write file with file name
	fp = fopen(filename, "w");

	//Check file pointer is open
	if(!fp) {
		perror("Error fopen");
		exit(-1);
	}
	
	//recieve port number for data transfer connection
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	
	//convert port number to int
	port = atoi(tempStr);
	
	//initialize data transfer connection
	datafd = initConn(ipAddr, port);
	
	//get connection confirmation message from server
	if(tcp_recv(datafd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	fprintf(stderr, "%s", tempStr);
	
	//print receiving message	
	fprintf(stderr, "Receiving file: %s (%d bytes)...\n", filename, fileSize);	
	
	//numRead: number of bytes read in single tcp packet
	//totalNumRead: total number of bytes read so far
	int numRead = 0;
	int totalNumRead = 0;
	
	//loop until entire file is received
	while(totalNumRead < fileSize) {
		
		//receive data from tcp connection
		if((numRead = tcp_recv(datafd, tempStr, min(MAX_MSG_SIZE, fileSize))) < 0) {
			perror("Error tcp_recv");
			exit(-1);
		}		
	
		//write data to the file
		if(fwrite(tempStr, sizeof(char), numRead, fp) < 0) {
			perror("Error fwrite");
			exit(-1);
		}
			
		//update total bytes read
		totalNumRead += numRead;
	}
	
	//close data connection
	close(datafd);
		
	//close file pointer
	fclose(fp);
}

void recv_ls(int connfd) {
	
	int numFiles = 0;
	char tempStr[MAX_MSG_SIZE];
	
	//receive file size from server
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_recv");
		exit(-1);
	}
	
	//convert port number to int
	numFiles = atoi(tempStr);
	
	//receive 
	while(numFiles--) {
		if(tcp_recv(connfd, tempStr, MAX_FILE_NAME_SIZE) < 0) {
			perror("Error tcp_recv");
			exit(-1);
		}
		
		//print receiving message	
		fprintf(stderr, "%s", tempStr);	
	}
}