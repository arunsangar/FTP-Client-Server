all: TCP helper client server makeserver

TCP:	TCP.h TCP.cpp
	g++ -c TCP.cpp

helper: helper.h helper.cpp
	g++ -c helper.cpp

client:	client.cpp
	g++ client.cpp helper.o TCP.o -o client

server: server.cpp
	g++ server.cpp helper.o TCP.o -o server

makeserver:
	rm -rf helper.o TCP.o
	mkdir -p ../FTPserver/Client
	mkdir -p ../FTPserver/Server
	mv client ../FTPserver/Client
	mv server ../FTPserver/Server

clean:
	rm -rf server client helper.o TCP.o