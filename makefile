all: client server

client:	client.cpp helper.o
	g++ client.cpp helper.o TCP.o -o client

server: server.cpp helper.o
	g++ server.cpp helper.o TCP.o -o server

helper: helper.cpp helper.o
	g++ -c helper.cpp

TCP.o:	TCP.h TCP.cpp
	g++ -c TCP.cpp 

clean:
	rm -rf server client TCP.o 