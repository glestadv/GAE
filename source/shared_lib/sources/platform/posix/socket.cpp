// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
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
#	define BSD_COMP /* needed for FIONREAD on Solaris2 */
#	include <sys/ioctl.h>
#endif

#if defined(HAVE_SYS_FILIO_H) /* needed for FIONREAD on Solaris 2.5 */
#	include <sys/filio.h>
#endif

#include "socket.h"
#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared { namespace Platform {

// ===============================================
// class Socket
// ===============================================


/**
 * Closes the socket ignoring any errors in the attempt.
 */
void Socket::close() throw() {
	::close(sock);
	sock = 0;
}

int Socket::getDataToRead() {
	unsigned long size;

	/* ioctl isn't posix, but the following seems to work on all modern
	* unixes */
	int err = ::ioctl(sock, FIONREAD, &size);

	if (err < 0 && errno != EAGAIN) {
		throw SocketException("Can not get data to read", "ioctl", NULL, __FILE__, __LINE__);
	}

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	ssize_t bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

	if (bytesSent != dataSize) {
		throw SocketException("failed to send data on socket", "send", NULL, __FILE__, __LINE__);
	}

	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize) {
	ssize_t bytesReceived = ::recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_DONTWAIT);

	if (bytesReceived < 0) {
		if (errno == EAGAIN) {
			bytesReceived = 0;
		} else {
			throw SocketException("error while receiving socket data", "recv", NULL, __FILE__, __LINE__);
		}
	}

	return static_cast<int>(bytesReceived);
}

int Socket::peek(void *data, int dataSize) {
	ssize_t bytesReceived = ::recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

	if (bytesReceived < 0) {
		if (errno == EAGAIN) {
			bytesReceived = 0;
		} else {
			throw SocketException("Can not receive data", "recv", NULL, __FILE__, __LINE__);
		}
	}

	return static_cast<int>(bytesReceived);
}

void Socket::setBlock(bool block) {
	if (::fcntl(sock, F_SETFL, block ? 0 : O_NONBLOCK) < 0) {
		throw SocketException("Error setting I/O mode for socket", "fcntl", NULL, __FILE__, __LINE__);
	}
}

bool Socket::isReadable() {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i = ::select(sock + 1, &set, NULL, NULL, &tv);
	if (i < 0) {
		throw SocketException("Error selecting socket", "select", NULL, __FILE__, __LINE__);
	}
	return i == 1;
}

bool Socket::isWritable() {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i = ::select(sock + 1, NULL, &set, NULL, &tv);
	if (i < 0) {
		throw SocketException("Error selecting socket", "select", NULL, __FILE__, __LINE__);
	}
	return i == 1;
}

// ===============================================
// class ClientSocket
// ===============================================

ClientSocket::ClientSocket(const IpAddress &remoteAddr, unsigned short remotePort)
		: Socket(SOCKET_TYPE_CLIENT)
		, remoteAddr(remoteAddr)
		, remotePort(remotePort) {

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remoteAddr.get();
	addr.sin_port = htons(remotePort);

	int ret = ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if (isError(ret)) {
		int err = getLastSocketError();
		if(err != EINPROGRESS) {
			throw SocketException("Error connecting socket", "connect", NULL, __FILE__, __LINE__);
		}
	}
}

// ===============================================
// class ServerSocket
// ===============================================

void ServerSocket::bind(unsigned short port) {
	//sockaddr structure
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	int err = ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (err < 0) {
		throw SocketException("Error binding socket", "bind", NULL, __FILE__, __LINE__);
	}
}

void ServerSocket::listen(int connectionQueueSize) {
	int err = ::listen(sock, connectionQueueSize);
	if (err < 0) {
		throw SocketException("Error listening socket", "listen", NULL, __FILE__, __LINE__);
	}
}

ClientSocket *ServerSocket::accept() {
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	int newSock = ::accept(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len);
	if (newSock < 0) {
		int err = errno;
		if (err == EWOULDBLOCK) {
			return NULL;
 		}

		throw SocketException("Error accepting socket connection", "accept", NULL, __FILE__, __LINE__, err);
	}

	return new ClientSocket(newSock, addr);
}

}}//end namespace

