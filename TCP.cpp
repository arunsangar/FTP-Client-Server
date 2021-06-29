#include "TCP.h"

int tcp_send(const int& socket, const void* buffer, const int& buffSize) {
	
	int numSent = 0, totalBytesSent = 0;
	
	while(totalBytesSent != buffSize) {
		if((numSent = write(socket, (char*)buffer + totalBytesSent, 
			buffSize - totalBytesSent)) < 0) return numSent;
		
		totalBytesSent += numSent;
	}
		
	return totalBytesSent;
}

int tcp_recv(const int& socket, void* buffer, const int& buffSize) {

	int numRecv = 0, totalBytesRecv = 0;
	
	while(totalBytesRecv != buffSize) {
		
		if((numRecv = read(socket, (char*)buffer + totalBytesRecv, 
			buffSize - totalBytesRecv)) < 0) return numRecv;
		
		totalBytesRecv += numRecv;
	}
	
	return totalBytesRecv;
}