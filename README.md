# FTP-Client-Server

**CPSC 474 - Parallel and Distributed Computing**\
**Project 2 - Parallel Maximum Sum Submatrix Variation**

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
