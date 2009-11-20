// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "socket.h"

#include <stdexcept>
#include <sstream>
#if ! ( defined(WIN32) || defined(WIN64) )
#include <netdb.h>
#else
#include <Ws2tcpip.h>
#endif
#include "leak_dumper.h"

using std::range_error;
using std::stringstream;

// Forward declaring class from the game project is hacky, but I want to control template
// instantiation for Socket::wait and haven't found a better way to do it :(
namespace Game { namespace Net {
class RemoteInterface;
}}

namespace Shared { namespace Platform {

// =====================================================
// class IpAddress
// =====================================================

IpAddress::IpAddress(const string &ipString) : addr(inet_addr(ipString.c_str())) {
	if(addr == (in_addr_t)(-1)) {
		throw range_error("String \"" + ipString + "\"cannot be converted into IPv4 Internet address.");
	}
}

string IpAddress::toString() const {
	char buf[16];
	const unsigned char *bytes = reinterpret_cast<const unsigned char *>(&addr);
	snprintf(buf, sizeof(buf), "%hhu.%hhu.%hhu.%hhu", bytes[0], bytes[1], bytes[2], bytes[3]);
	return string(buf);
}

// =====================================================
// class Socket
// =====================================================

Socket::Socket(SocketType type)
		: sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
		, type(type) {
	if (!isValid(sock)) {
		throw  SocketException("Error creating socket", "::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)", NULL, __FILE__, __LINE__);
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

Socket::~Socket() throw () {
	if(sock) {
		close();
	}
}

bool Socket::isConnected() {

	//if the socket is not writable then it is not conencted
	if (!isWritable()) {
		return false;
	}

	//if the socket is readable it is connected if we can read a byte from it
	if (isReadable()) {
		char tmp;
		return ::recv(sock, &tmp, sizeof(tmp), MSG_PEEK) > 0;
	}

	//otherwise the socket is connected
	return true;
}

string Socket::getLocalHostName() const {
	char hostname[1024];
	int ret = gethostname(hostname, sizeof(hostname));
	if(isError(ret)) {
		throw SocketException("Error getting local host name.", "gethostname",
				NULL, __FILE__, __LINE__);
	}
	return string(hostname);
}

// FIXME: This is hacky and ignores several concepts, like multi-homing and prone to problems with
// hostname configuration errors
string Socket::getIpAddress() const {
	hostent* info = ::gethostbyname(getLocalHostName().c_str());
	unsigned char* address;

	if (info == NULL) {
		throw SocketException("Error getting host by name", "gethostbyname", NULL, __FILE__, __LINE__);
	}

	address = reinterpret_cast<unsigned char*>(info->h_addr_list[0]);

	if (address == NULL) {
		throw SocketException("Error getting host ip", "gethostbyname", NULL, __FILE__, __LINE__);
	}

	std::stringstream ret;
	ret		<< static_cast<int>(address[0]) << "."
			<< static_cast<int>(address[1]) << "."
			<< static_cast<int>(address[2]) << "."
			<< static_cast<int>(address[3]);
	return ret.str();
}


sockaddr_in Socket::getsockname() const {
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	int ret = ::getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len);

	if(isError(ret)) {
		throw SocketException("Retreving local address from socket", "getsockname",
				NULL, __FILE__, __LINE__);
	}
	return addr;
}

template<typename T>
void Socket::wait(vector<SocketTestData<T> > &sockets, size_t millis) {
	fd_set read, write, error;
	struct timeval maxTime;
	SOCKET maxfd = 0;
	FD_ZERO(&read);
	FD_ZERO(&write);
	FD_ZERO(&error);
	maxTime.tv_sec = millis / 1000;
	maxTime.tv_usec = (millis % 1000) * 1000;

	for(typename vector<SocketTestData<T> >::iterator i = sockets.begin(); i != sockets.end(); ++i) {
		i->result = SOCKET_TEST_NONE;
		SOCKET sock = i->socket->sock;
		// if it's a bad/closed socket, we set it to error.
		if(!sock) {
			i->result = SOCKET_TEST_ERROR;
		} else {
			if(sock > maxfd) {
				maxfd = sock;
			}
			if(i->test & SOCKET_TEST_READ) {
				FD_SET(sock, &read);
			}
			if(i->test & SOCKET_TEST_WRITE) {
				FD_SET(sock, &write);
			}
			if(i->test & SOCKET_TEST_ERROR) {
				FD_SET(sock, &error);
			}
		}
	}

	if(!maxfd) {
		// no good sockets
		return;
	} else {
		if(::select(maxfd + 1, &read, &write, &error, &maxTime) == SOCKET_ERROR) {
			throw SocketException("Error selecting socket",
					"select(maxfd + 1, &read, &write, &error, &maxTime)",
					NULL, __FILE__, __LINE__);
		}
	}

	for(typename vector<SocketTestData<T> >::iterator i = sockets.begin(); i != sockets.end(); ++i) {
		SOCKET sock = i->socket->sock;
		if(!sock) {
			continue;
		}
		if(FD_ISSET(sock, &read)) {
			i->result = SOCKET_TEST_READ;
		} else {
			i->result = SOCKET_TEST_NONE;
		}
		if(FD_ISSET(sock, &write)) {
			i->result = i->result | SOCKET_TEST_WRITE;
		}
		if(FD_ISSET(sock, &error)) {
			i->result = i->result | SOCKET_TEST_ERROR;
		}
	}
}
// explicit template instantiations:
template void Socket::wait(vector<SocketTestData<Game::Net::RemoteInterface *> > &, size_t);

// =====================================================
// class ClientSocket
// =====================================================

string ClientSocket::getRemoteHostName() const {
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	int ret = getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len);
	if(isError(ret)) {
		throw SocketException("Retreving remote address from socket", "getpeername",
				NULL, __FILE__, __LINE__);
	}

	assert(addr.sin_family == AF_INET);

	char buf[1024];
	ret = getnameinfo(reinterpret_cast<sockaddr*>(&addr), addr_len, buf, sizeof(buf), NULL, 0, 0);
	if(isError(ret)) {
		throw SocketException("Retreving remote host name", "getnameinfo",
				NULL, __FILE__, __LINE__);
	}
	return string(buf);
}


}}//end namespace