// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <stdexcept>
#include <sstream>
#if defined(HAVE_SYS_IOCTL_H)
#define BSD_COMP /* needed for FIONREAD on Solaris2 */
#include <sys/ioctl.h>
#endif
#if defined(HAVE_SYS_FILIO_H) /* needed for FIONREAD on Solaris 2.5 */
#include <sys/filio.h>
#endif

#include "socket.h"
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
//	class Socket::CircularBuffer
// =====================================================

/** record that we have added b bytes to the buffer */
void Socket::CircularBuffer::operator+=(size_t b) {
	assert(b && head + b <= buffer_size);
	head += b;
	if (head == buffer_size) head = 0;
	if (head == tail) full = true;
}

/** record that we have removed b bytes from the buffer */
void Socket::CircularBuffer::operator-=(size_t b) {
	assert(b);
	if (tail + b <= buffer_size) {
		tail += b;
		if (tail == buffer_size) tail = 0;
	} else {
		tail = b - (buffer_size - tail);
	}
	if (full) full = false;
}

/** Number of bytes available to read */
size_t Socket::CircularBuffer::bytesAvailable() const {
	if (full) {
		return buffer_size;
	} else if (head < tail) {
		return head + buffer_size - tail;
	} else {
		return head - tail;
	}
}

/** peek the next n bytes, copying them to dst if the request can be satisfied
  * @return true if ok, false if not enough bytes are available */
bool Socket::CircularBuffer::peekBytes(void *dst, size_t n) {
	if (bytesAvailable() < n) {
		return false;
	}
	char_ptr ptr = (char_ptr)dst;
	if (tail + n <= buffer_size) {
		memcpy(ptr, buffer + tail, n);
	} else {
		size_t first_n = buffer_size - tail;
		size_t second_n = n - first_n;
		memcpy(ptr, buffer + tail, first_n);
		memcpy(ptr + first_n, buffer, second_n);
	}
	return true;
}

/** read n bytes to dst, advancing tail offset ('removing' them from the buffer) 
  * @return true if all ok, false if not enough bytes available, in which case none will be read */
bool Socket::CircularBuffer::readBytes(void *dst, int n) {
	if (peekBytes(dst, n)) {
		*this -= n;
		return true;
	}
	return false;
}

/** returns the maximum write length from the head, if no more bytes can be written
  * after this (because the head would be at the tail) limit is set to true */
int Socket::CircularBuffer::getMaxWrite(bool &limit) const {
	if (full) {
		limit = true;	
		return 0;
	}
	if (tail > head) { // chasing tail?
		limit = true;
		return tail - head;
	} else {
		limit = tail ? false : true;
		return buffer_size - head;
	}
}

/** free space in buffer */
size_t Socket::CircularBuffer::getFreeBytes() const {
	if (full) return 0;
	if (tail > head) {
		return tail - head;
	} else {
		return buffer_size - head + tail; 
	}
}

// ===============================================
//	class Socket
// ===============================================

#define SOCKET_ERROR (-1)

Socket::Socket(int sock){
	this->sock= sock;
}

Socket::Socket(){
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0) {
		throw SocketException("Error creating socket");
	}
	// some basic sanity checks on our types
	assert(sizeof(int8) == 1);
	assert(sizeof(uint8) == 1);
	assert(sizeof(int16) == 2);
	assert(sizeof(uint16) == 2);
	assert(sizeof(int32) == 4);
	assert(sizeof(uint32) == 4);
	assert(sizeof(int64) == 8);
	assert(sizeof(uint64) == 8);
}

Socket::~Socket() {
	::close(sock);
}

int Socket::getDataToRead() {
	readAll();
	return buffer.bytesAvailable();
}

int Socket::send(const void *data, int dataSize) {
	ssize_t bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

	if(bytesSent != dataSize) {
		if (bytesSent == SOCKET_ERROR) {
			handleError(__FUNCTION__);
		} else {
			throw SocketException("failed to send data on socket");
		}
	}
	return int(bytesSent);
}

int Socket::receive(void *data, int dataSize) {
	readAll();
	if (buffer.readBytes(data, dataSize)) {
		return dataSize;
	}
	return 0;
}

int Socket::peek(void *data, int dataSize) {
	readAll();
	if (buffer.peekBytes(data, dataSize)) {
		return dataSize;
	}
	return 0;
}

void Socket::setBlock(bool block) {
	int err = fcntl(sock, F_SETFL, block ? 0 : O_NONBLOCK);

	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

bool Socket::isReadable(){
	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, &set, NULL, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return i==1;
}

bool Socket::isWritable(){
	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, NULL, &set, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return i==1;
}

bool Socket::isConnected(){

	//if the socket is not writable then it is not conencted
	if(!isWritable()){
		return false;
	}

	// All input is 'drained' into our own buffer now, this needs sorting out
	// this would be unreliable (I think...) 
	//if the socket is readable it is connected if we can read a byte from it
	if(isReadable()){
	//	char tmp;
		return true;// recv(sock, &tmp, sizeof(tmp), MSG_PEEK) > 0;
	}

	//otherwise the socket is connected
	return true;
}

void Socket::readAll() {
	bool limit;
	int n = buffer.getMaxWrite(limit);
	int r = recv(sock, buffer.getWritePos(), n, 0);
	if (r == SOCKET_ERROR) {
		if (errno != EAGAIN) {
			handleError(__FUNCTION__);
		}
	}
	if (r > 0) {
		buffer += r;
		if (r == n && !limit) {
			n = buffer.getMaxWrite(limit);
			assert(n);
			int r2 = recv(sock, buffer.getWritePos(), n, 0);
			if (r2 > 0) {
				buffer += r2;
			} else if (r2 == SOCKET_ERROR && errno != EAGAIN) {
				handleError(__FUNCTION__);
			}
			//cout << "Socket::readALL() read " << r + r2 << " bytes.\n";
			return;
		}
		//cout << "Socket::readALL() read " << r << " bytes.\n";
	}
}


string Socket::getHostName() const {
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() const{
	hostent* info= gethostbyname(getHostName().c_str());
	unsigned char* address;

	if(info==NULL){
		throw runtime_error("Error getting host by name");
	}

	address= reinterpret_cast<unsigned char*>(info->h_addr_list[0]);

	if(address==NULL){
		throw runtime_error("Error getting host ip");
	}

	return
		intToStr(address[0]) + "." +
		intToStr(address[1]) + "." +
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::handleError(const char *caller) const {
	std::stringstream msg;
	msg << "Socket Error in : " << caller << " [Error code: " << errno << "]\n";
	msg << strerror(errno);
	throw SocketException(msg.str());
}

// ===============================================
//	class ClientSocket
// ===============================================

void ClientSocket::connect(const Ip &ip, int port){
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

// ===============================================
//	class ServerSocket
// ===============================================

void ServerSocket::bind(int port){
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

void ServerSocket::listen(int connectionQueueSize){
	int err= ::listen(sock, connectionQueueSize);
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

Socket *ServerSocket::accept(){
	int newSock= ::accept(sock, NULL, NULL);
	if(newSock < 0) {
		if(errno == EAGAIN) {
			return NULL;
		}
		handleError(__FUNCTION__);
	}
	return new Socket(newSock);
}

}}//end namespace

