// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "socket.h"

#include <stdexcept>

#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

// =====================================================
//	class Ip
// =====================================================

Ip::Ip(){
	bytes[0]= 0;
	bytes[1]= 0;
	bytes[2]= 0;
	bytes[3]= 0;
}

Ip::Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3){
	bytes[0]= byte0;
	bytes[1]= byte1;
	bytes[2]= byte2;
	bytes[3]= byte3;
}


Ip::Ip(const string& ipString){
	int offset= 0;
	int byteIndex= 0;

	for(byteIndex= 0; byteIndex<4; ++byteIndex){
		int dotPos= ipString.find_first_of('.', offset);

		bytes[byteIndex]= atoi(ipString.substr(offset, dotPos-offset).c_str());
		offset= dotPos+1;
	}
}

string Ip::getString() const{
	return intToStr(bytes[0]) + "." + intToStr(bytes[1]) + "." + intToStr(bytes[2]) + "." + intToStr(bytes[3]);
}

// =====================================================
//	class Socket
// =====================================================

Socket::LibraryManager Socket::libraryManager;

Socket::LibraryManager::LibraryManager(){
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAStartup(wVersionRequested, &wsaData);
	//dont throw exceptions here, this is a static initializacion
}

Socket::LibraryManager::~LibraryManager(){
	WSACleanup();
}

Socket::Socket(SOCKET sock){
	this->sock= sock;
}

Socket::Socket(){
	sock= socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sock==INVALID_SOCKET){
		throw runtime_error("Error creating socket");
	}
	assert(sizeof(int8) == 1);
	assert(sizeof(uint8) == 1);
	assert(sizeof(int16) == 2);
	assert(sizeof(uint16) == 2);
	assert(sizeof(int32) == 4);
	assert(sizeof(uint32) == 4);
	assert(sizeof(int64) == 8);
	assert(sizeof(uint64) == 8);
}

Socket::~Socket(){
	int err = closesocket(sock);
	if (err) {
		handleError(__FUNCTION__);
	}
}

int Socket::getDataToRead(){
	u_long size;
	int err = ioctlsocket(sock, FIONREAD, &size);
	if (err == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			return 0;
		}
		handleError(__FUNCTION__);
	}
	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	int err = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	if (err == SOCKET_ERROR) {
		if(WSAGetLastError() != WSAEWOULDBLOCK){
			handleError(__FUNCTION__);
		}
	}
	return err;
}

int Socket::receive(void *data, int dataSize) {
	int err= recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
	if (err == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			handleError(__FUNCTION__);
		}
	}
	return err;
}

int Socket::peek(void *data, int dataSize) {
	int err= recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
	if (err == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			handleError(__FUNCTION__);
		}
	}
	return err;
}

void Socket::setBlock(bool block) {
	u_long iMode = !block;
	int err = ioctlsocket(sock, FIONBIO, &iMode);
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

bool Socket::isReadable() {
	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(0, &set, NULL, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return (i == 1);
}

bool Socket::isWritable() {
	TIMEVAL tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i = select(0, NULL, &set, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return (i == 1);
}

bool Socket::isConnected() {
	//if the socket is not writable then it is not conencted
	if (!isWritable()) {
		return false;
	}
	//if the socket is readable it is connected if we can read a byte from it
	if (isReadable()) {
		char tmp;
		return (recv(sock, &tmp, sizeof(tmp), MSG_PEEK) > 0);
	}
	//otherwise the socket is connected
	return true;
}

string Socket::getHostName() const{
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() const {
	hostent* info = gethostbyname(getHostName().c_str());
	if (!info) {
		handleError(__FUNCTION__);
	}
	unsigned char* address = 
		reinterpret_cast<unsigned char*>(info->h_addr_list[0]);
	if (!address) {
		throw runtime_error("Error getting host ip");
	}
	return
		intToStr(address[0]) + "." +
		intToStr(address[1]) + "." +
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::handleError(const char *caller) const {
	const char *msg_ptr = 0;
	int errCode = WSAGetLastError();
	DWORD frmtFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
	int msgRes = FormatMessage(frmtFlags,NULL, errCode, 0, (LPSTR)msg_ptr, 1024, NULL);
	string msg = string("Socket Error in : ") + caller;
	if (msgRes) {
		msg += "\n" + string(msg_ptr);
		LocalFree((HLOCAL)msg_ptr);
	} else {
		msg += "\nFormatMessage() call failed. :~(";
	}	
	throw runtime_error(msg);
}

// =====================================================
//	class ClientSocket
// =====================================================

void ClientSocket::connect(const Ip &ip, int port) {
	sockaddr_in addr;

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if (err==SOCKET_ERROR) {
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK && errCode != WSAEALREADY) {
			handleError(__FUNCTION__);
		}
	}
}

// =====================================================
//	class ServerSocket
// =====================================================

void ServerSocket::bind(int port) {
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int err = ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

void ServerSocket::listen(int connectionQueueSize) {
	int err = ::listen(sock, connectionQueueSize);
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

Socket *ServerSocket::accept() {
	SOCKET newSock = ::accept(sock, NULL, NULL);
	if (newSock == INVALID_SOCKET) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			return NULL;
		}
		handleError(__FUNCTION__);
	}
	return new Socket(newSock);
}

}}//end namespace
