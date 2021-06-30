# FTP-Client-Server

**CPSC 471 - Computer Communications**\
**Project 1 - FTP Client and Server**

### **Problem description:**

Develop a simplified FTP client and server program capable of processing basic commands and sending/receiving files.

### **Goals:**

1. Understand the challenges of protocol design.
2. Understand the challenges of developing real-world network applications.
3. Practice with socket programming APIs.

### **How it works:**

This program features a basic FTP server and client program. Once the FTP server is running, the client program can open a session and run basic commands. There are 4 available commands:

1. ls - list the files in the server's directory
2. get - asks the server to send a file
3. put - send a file to the server
4. quit - close the session with the server

---

### **Running the program:**

A makefile is available in the code folder. It can be used to make a seperate FTPserver directory. This directory will keep the client and server programs in seperate folders.

```
make all
```

Compile:

```
g++ -c TCP.cpp
g++ -c helper.cpp
g++ client.cpp helper.o TCP.o -o client
g++ server.cpp helper.o TCP.o -o server
```

Execute:

```
./server <port>
./client <server ip> <port>
```

### **Command line arguments:**

port - port number used for the TCP socket\
server ip - IPv4 address of the server

### **Constraints:**

**TODO**

---

### **Test files:**

1. small.txt - 'Hello World!'
2. big.txt - The Three Musketeers ebook

### **Links:**

[GeeksForGeeks - File Transfer Protocol (FTP) in Application Layer](https://www.geeksforgeeks.org/file-transfer-protocol-ftp-in-application-layer/)\
[Techopedia - FTP Server](https://www.techopedia.com/definition/26108/ftp-server)\
[FTP Today - How does an FTP server work and what are its benefits?](https://www.ftptoday.com/blog/how-does-an-ftp-server-work-the-benefits)
