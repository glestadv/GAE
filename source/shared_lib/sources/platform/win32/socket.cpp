// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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
#include "platform_exception.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared { namespace Platform {

// =====================================================
// class Socket
// =====================================================

Socket::LibraryManager Socket::libraryManager;

Socket::LibraryManager::LibraryManager() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAStartup(wVersionRequested, &wsaData);
	//dont throw exceptions here, this is a static initializacion
}

Socket::LibraryManager::~LibraryManager() {
	WSACleanup();
}

void Socket::close() throw() {
	int ret = closesocket(sock);
/*
	if (ret == INVALID_SOCKET) {
		throw WinsockException("Error closing socket", "closesocket", NULL, __FILE__, __LINE__);
	}*/
	sock = 0;
}

int Socket::getDataToRead() {
	u_long size;
	int ret = ioctlsocket(sock, FIONREAD, &size);

	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			throw WinsockException("Can not get data to read", "ioctlsocket", NULL, __FILE__, __LINE__, err);
		}
	}

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	int ret = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			throw WinsockException("Can not send data", "send", NULL, __FILE__, __LINE__, err);
		}
	}
	return ret;
}

int Socket::receive(void *data, int dataSize) {
	int ret = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			throw WinsockException("Can not receive data", "recv", NULL, __FILE__, __LINE__, err);
		}
	}

	return ret;
}

int Socket::peek(void *data, int dataSize) {
	int ret = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			throw WinsockException("Can not receive data", "recv", NULL, __FILE__, __LINE__, err);
		}
	}

	return ret;
}

void Socket::setBlock(bool block) {
	u_long iMode = !block;
	int ret = ioctlsocket(sock, FIONBIO, &iMode);

	if (ret == SOCKET_ERROR) {
		throw WinsockException("Error setting I/O mode for socket", "ioctlsocket", NULL, __FILE__, __LINE__);
	}
}

bool Socket::isReadable() {
	TIMEVAL tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i = select(0, &set, NULL, NULL, &tv);
	if (i == SOCKET_ERROR) {
		throw WinsockException("Error selecting socket", "select", NULL, __FILE__, __LINE__);
	}
	return i == 1;
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
		throw WinsockException("Error selecting socket", "select", NULL, __FILE__, __LINE__);
	}
	return i == 1;
}

// =====================================================
// class ClientSocket
// =====================================================

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

	if (ret == SOCKET_ERROR) {
		int lastError = WSAGetLastError();

		if (lastError != WSAEWOULDBLOCK && lastError != WSAEALREADY) {
			throw WinsockException("Can not connect", "connect", NULL, __FILE__, __LINE__, lastError);
		}
	}
}

// =====================================================
// class ServerSocket
// =====================================================

void ServerSocket::bind(unsigned short port) {
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
		throw WinsockException("Error binding socket", "bind", NULL, __FILE__, __LINE__);
	}
}

void ServerSocket::listen(int connectionQueueSize) {
	if (::listen(sock, connectionQueueSize) == SOCKET_ERROR) {
		throw WinsockException("Error listening socket", "listen", NULL, __FILE__, __LINE__);
	}
}

ClientSocket *ServerSocket::accept() {
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	SOCKET newSock = ::accept(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len );
	if (newSock == INVALID_SOCKET) {
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK) {
			return NULL;
		}
		throw WinsockException("Error accepting socket connection", "accept", NULL, __FILE__, __LINE__, err);
	}
	return new ClientSocket ( newSock, addr );
}

}} // end namespace

