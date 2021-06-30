#include "helper.h"

#define MAX_DIR_SIZE 200

void listenControl(int port);
int  control(int connfd, char* ipAddr);
void send_file(int connfd, char* ipAddr, char* filename);
void recv_file(int connfd, char* filename);
void send_ls(int connfd);
int  listenData(int connfd);

int main(int argc, char** argv) {
	
	//verify command line parameters
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <PORT #>", argv[0]);
		exit(-1);	
	}

	//port: port number
	int port = -1;
	
	//convert port number to integer
	port = atoi(argv[1]);
	
	//verify port number is within range
	if(port < 0 || port > 65535) {
		fprintf(stderr, "Error: invalid port number\n");
		exit(-1);
	}
	
	//listen for incoming control session requests
	listenControl(port);
	
	return 0;
}

void listenControl(int port) {
	
	//ipAddr: server IP address
	char ipAddr[15] = "";
	
	//listenfd: listen socket file descriptor
	//connfd: connection socket file descriptor
	int listenfd = -1;
	int connfd = -1;
	
	//serverAddr: server address struct
	//cliAddr: client address struct
	//cliLen: size of client address
	sockaddr_in serverAddr, cliAddr;
	socklen_t cliLen = sizeof(cliAddr);
	
	//print port number
	fprintf(stderr, "Connecting to port %d\n", port); 
	
	//create socket to listen for incoming connections (TCP - reliable)
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error socket");
		exit(-1);
	}
	
	//initialize server address struct
	memset(&serverAddr, 0, sizeof(serverAddr));
		
	//convert port number to network byte order
	serverAddr.sin_port = htons(port);
	
	//set server family
	serverAddr.sin_family = AF_INET;
	
	//retrieve packets from all network interfaces
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//Associate address (IP + port) with listen socket file descriptor
	if(bind(listenfd, (sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		perror("Error bind");
		exit(-1);
	}
	
	//listen for incoming connections (passive socket)
	//supports 1 connection
	if(listen(listenfd, 1) < 0) {
		perror("Error listen");
		exit(-1);
	}
	
	//infinite loop: listen for incoming connections forever
	while(true) {
		
		//begun listening message
		fprintf(stderr, "Listening on port %d for incoming connection\n", port);
				
		//connection file descriptor to store client address
		if((connfd = accept(listenfd, (sockaddr *)&cliAddr, &cliLen)) < 0) {
			perror("Error accept");
			exit(-1);
		}
		
		//get the IP address in string format
		if(inet_ntop(AF_INET, &(cliAddr.sin_addr), ipAddr, INET_ADDRSTRLEN) == NULL) {
			perror("Error inet_ntop");
			exit(-1);
		}
		
		//print successful connection message
		fprintf(stderr, "Success: connection acquired\n");
		fprintf(stderr, "Connected to: %s\nPort Number: %d\n", ipAddr, port);
		fprintf(stderr, "Control session started\n");
		
		//send to control loop to handle session
		control(connfd, ipAddr);	
	}
	
	//close listen connection
	close(listenfd);
	
	//close control connection
	close(connfd);
}
	
int control(int connfd, char* ipAddr) {
	
	//sendMsg: response message sent to client (return codes)
	//cmdMsg: command message from client (command + filename)
	//cmd: command (extracted from the command message)
	//filename: name of file to be transferred (extracted from command message)
	char sendMsg[MAX_MSG_SIZE] = "";
	char cmdMsg[MAX_CMD_MSG_SIZE] = "";
	char* cmd;
	char* filename;
	
	//openSession: tracks if client has asked to end session
	bool openSession = true;
	
	//send a confirmation response to client
	strcpy(sendMsg, "800 Success: connection established\n");
	if(tcp_send(connfd, sendMsg, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	while(openSession)
	{
		//receive command message from client
		if(tcp_recv(connfd, cmdMsg, MAX_CMD_MSG_SIZE) < 0) {
			perror("Error tcp_recv");
			exit(-1);
		}
		
		//extract command from command message
		cmd = strtok(cmdMsg, "|");
		
		//check if command is get/put
		if(strcmp(cmd, "get") == 0 || strcmp(cmd, "put") == 0) {
			
			//extract filename
			filename = strtok(NULL, "|");
			
			//execute get command
			if(strcmp(cmd, "get") == 0) {
				send_file(connfd, ipAddr, filename);
				strcpy(sendMsg, "801 Success: get ");
			}
			//execute put command
			else {
				recv_file(connfd, filename);
				strcpy(sendMsg, "803 Success: put ");
			}
			
			//print data connection close message
			fprintf(stderr, "Data transfer session closed\n");
			
			//add file name to response message
			strcat(sendMsg, filename);
		}
		
		//execute ls command
		else if(strcmp(cmd, "ls") == 0) {
			send_ls(connfd);
			strcpy(sendMsg, "805 Success: ls");
		}
		
		//execute quit command
		else if(strcmp(cmd, "quit") == 0) {
			openSession = false;
			strcpy(sendMsg, "807 Success: quit");
		}
		
		//send response message
		strcat(sendMsg,"\n");
		if(tcp_send(connfd, sendMsg, MAX_MSG_SIZE) < 0) {
			perror("Error tcp_send");
			exit(-1);
		}
		
		//clear contents of buffers cmd and filename
		memset(cmd,0,sizeof(cmd));
		memset(filename,0,sizeof(filename));
		
		//close the control session
		if(openSession == false) {
			if(close(connfd) < 0) {
				perror("Error: close connection");
				exit(-1);
			}
		}
	}
	return 0;
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
	
	//send file size to client
	if(tcp_send(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	datafd = listenData(connfd);
	
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

void recv_file(int connfd, char* filename) {
	
	//tempStr: temporary message string
	char tempStr[MAX_MSG_SIZE];
	
	//fileSize: size of file
	//port: ephemeral port number
	//datafd: data transfer connection socket
	int fileSize = 0;
	int port = -1;
	int datafd = -1;
	
	//receive file size from client
	if(tcp_recv(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tecp_recv");
		exit(-1);
	}
	
	//convert file size from string to integer
	fileSize = atoi(tempStr);
	
	//print receiving message	
	fprintf(stderr, "Receiving file: %s (%d bytes)...\n", filename, fileSize);	
		
	//file pointer for write file
	FILE *fp;
	
	//open write file with file name
	fp = fopen(filename, "w");

	//Check file pointer is open
	if(!fp) {
		perror("Error fopen");
		exit(-1);
	}
	
	//listen for incoming data connection
	datafd = listenData(connfd);
	
	//numRead: number of bytes read in single tcp packet
	//totalNumRead: total number of bytes read so far
	int numRead = 0;
	int totalNumRead = 0;
	
	//print receiving message	
	fprintf(stderr, "Receiving file: %s (%d bytes)...\n", filename, fileSize);	

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

void send_ls(int connfd) {
	
	//
	char lscmd[MAX_MSG_SIZE + 10] = "/bin/ls ";
	char cwd[MAX_DIR_SIZE] = "";
	
	FILE *fp;
	char tempStr[MAX_FILE_NAME_SIZE];
	
	int numFiles = 0;
	
	getcwd(cwd, sizeof(cwd));
	strcat(lscmd, cwd);

	/* Open the command for reading. */
	fp = popen(lscmd, "r");
	
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

	/* Read the output a line at a time - output it. */
	while (fgets(tempStr, sizeof(tempStr)-1, fp) != NULL) numFiles++;
	
	//convert file size to string format and save to tempStr
	if(snprintf(tempStr, MAX_MSG_SIZE, "%d", numFiles) < 0) {
		perror("Error snprintf");
		exit(-1);
	}
	
	//send file size to client
	if(tcp_send(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	pclose(fp);
	fp = popen(lscmd, "r");
	
	fprintf(stderr, "%s", "Sending directory list...\n");
	
	while (fgets(tempStr, sizeof(tempStr)-1, fp) != NULL) {
		//send file size to client
		if(tcp_send(connfd, tempStr, MAX_FILE_NAME_SIZE) < 0) {
			perror("Error tcp_send");
			exit(-1);
		}
	}
		
	/* close */
	pclose(fp);
}

int listenData(int connfd) {
	
	//tempStr: temporary message string
	char tempStr[MAX_MSG_SIZE];
	
	//listenfd: listen socket file descriptor
	//datafd: data transfer socket file descriptor
	int listenfd = -1;
	int datafd = -1;
	
	//port: port number
	int port = -1;
	
	//serverAddr: server address struct
	//cliAddr: client address struct
	//randomPortAddr: used to extract ephemeral port number
	//cliLen: size of client address
	sockaddr_in serverAddr, cliAddr, randomPortAddr;
	socklen_t cliLen = sizeof(cliAddr);	
	
	//create socket to listen for incoming connections (TCP - reliable)
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error socket");
		exit(-1);
	}
	
	//initialize server address struct
	memset(&serverAddr, 0, sizeof(serverAddr));
		
	//convert port number to network byte order
	//port number 0 creates ephemeral port
	serverAddr.sin_port = htons(0);
	
	//set server family
	serverAddr.sin_family = AF_INET;
	
	//retrieve packets from all network interfaces
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//get size of address
	socklen_t addrSize = sizeof(serverAddr);
	
	//associate address (IP + port) with listen socket file descriptor
	if(bind(listenfd, (sockaddr *) &serverAddr, addrSize) < 0) {
		perror("Error bind");
		exit(-1);
	}
	
	//retrieve ephemeral port number
	if(getsockname(listenfd, (struct sockaddr*)&randomPortAddr, &addrSize) < 0) {
		perror("getsockname");
		exit(-1);
	}
	
	//get the port number
	port = ntohs(randomPortAddr.sin_port);
	
	//convert port number
	if(snprintf(tempStr, MAX_MSG_SIZE, "%d", port) < 0) {
		perror("Error snprintf");
		exit(-1);
	}
	
	//send port number to client
	if(tcp_send(connfd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	//listen for incoming connections (passive socket)
	//supports 1 connection
	if(listen(listenfd, 1) < 0) {
		perror("Error listen");
		exit(-1);
	}
	
	//accept connection from client
	if((datafd = accept(listenfd, (sockaddr *)&cliAddr, &cliLen)) < 0) {
		perror("Error accept");
		exit(-1);
	}
	
	//print connection message
	fprintf(stderr, "Success: connection acquired\n");
	fprintf(stderr, "Port Number: %d\n", port);
	fprintf(stderr, "Data transfer session started\n");
	
	//send confirmation message
	strcpy(tempStr, "800 Success: connection established\n");
	if(tcp_send(datafd, tempStr, MAX_MSG_SIZE) < 0) {
		perror("Error tcp_send");
		exit(-1);
	}
	
	//close the listen connection
	close(listenfd);
	
	//return the data connection
	return datafd;
}

