// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_SOCKET_H_
#define _SHARED_PLATFORM_SOCKET_H_

#ifdef DEBUG_NETWORK_DELAY
#	define DEBUG_NETWORK_DELAY_BOOL true
#else
#	define DEBUG_NETWORK_DELAY_BOOL false
#endif
#include <string>
#include <cassert>
#include <vector>

#ifdef USE_POSIX_SOCKETS
#	include <unistd.h>
#	include <errno.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <fcntl.h>
#
#	define SOCKET_ERROR -1
#	define SocketException PosixException
#elif defined(WIN32) || defined(WIN64)
#	define SocketException WinsockException
#endif

#include "types.h"
#include "util.h"
#include "net_util.h"
#include "platform_exception.h"

using std::string;
using std::vector;

namespace Shared { namespace Platform {

// =====================================================
//	class IP
// =====================================================

/**
 * An IPv4 address.  Note that this is not IPv6 compliant in the slightest.
 */
class IpAddress : public NetSerializable {
private:
	in_addr_t addr;

public:
	/** Create an empty IPv4 address (i.e., 0.0.0.0). */
	IpAddress() : addr(0) {}
	
 	IpAddress(const sockaddr_in &addr) : addr(addr.sin_addr.s_addr) {
		assert(addr.sin_family == AF_INET);
	}
	
	IpAddress(in_addr_t addr) : addr(addr) {}
	
	/** Create a new IPv4 address by parsing the string ipString. */
	IpAddress(const string& ipString);

	/** Returns true if this address is 0.0.0.0. */
	bool isZero() const							{return !addr;}
	
	/** Converts this address to a string. */
	string toString() const;
	in_addr_t get() const						{return addr;}

	// NetSerializable member functions
	size_t getNetSize() const					{return sizeof(addr);}
	size_t getMaxNetSize() const				{return sizeof(addr);}
	void read(NetworkDataBuffer &buf)			{buf.read(addr);}
	void write(NetworkDataBuffer &buf) const	{buf.write(addr);}
};

enum SocketTest {
	SOCKET_TEST_NONE	= 0x00,
	SOCKET_TEST_READ	= 0x01,
	SOCKET_TEST_WRITE	= 0x02,
	SOCKET_TEST_ERROR	= 0x04
};

class Socket;
template<typename T> class SocketTestData {
public:
	Socket *socket;
	int test;
	int result;
	T value;
	SocketTestData(Socket *socket, int test, T value)
			: socket(socket)
			, test(test)
			, result(0)
			, value(value) {
	}
};

// =====================================================
//	class Socket
// =====================================================

class Socket {
public:
	enum SocketType {
		SOCKET_TYPE_CLIENT,
		SOCKET_TYPE_SERVER
	};

#if defined(WIN32) || defined(WIN64)
	class LibraryManager {
	public:
		LibraryManager();
		~LibraryManager();
	};

	static LibraryManager libraryManager;
#endif

protected:
	SOCKET sock;
	SocketType type;

	Socket(SOCKET sock, SocketType type)
		: sock(sock)
		, type(type) {
	}
	Socket(SocketType type);

public:
	virtual ~Socket() throw();

	//TODO: Move read & write functions to ClientSocket since they can never be called with a
	// ServerSocket?
	int getDataToRead();
	int send(const void *data, int dataSize);
	int receive(void *data, int dataSize);
	int peek(void *data, int dataSize);
	void close() throw();
	bool isOpen() const		{return sock;}

	void setBlock(bool block);
	bool isReadable();
	bool isWritable();
	bool isConnected();
	bool isClient() const	{return type == SOCKET_TYPE_CLIENT;}
	bool isServer() const	{return type == SOCKET_TYPE_SERVER;}

	string getLocalHostName() const;
	string getIpAddress() const;

	sockaddr_in getsockname() const;
	unsigned int getLocalPort() const	{return ntohs(getsockname().sin_port);}
	IpAddress getLocalAddress() const	{return IpAddress(getsockname());}
	
//	void close() {::close(sock);}

	template<typename T>
	static void wait(vector<SocketTestData<T> > &sockets, size_t millis);

#if defined(WIN32) || defined(WIN64)
	static bool isValid(SOCKET s)		{return s != INVALID_SOCKET;}
	static bool isError(int result)		{return result == SOCKET_ERROR;}
	static int getLastSocketError()		{return WSAGetLastError();}
#else
	static bool isValid(SOCKET s)		{return s >= 0;}
	static bool isError(int result)		{return result < 0;}
	static int getLastSocketError()		{return errno;}
#endif
};

// =====================================================
//	class ClientSocket
// =====================================================

class ClientSocket : public Socket {
protected:
	ClientSocket(SOCKET sock, const sockaddr_in &addr)
			: Socket(sock, SOCKET_TYPE_CLIENT)
			, remoteAddr(addr)
			, remotePort(ntohs(addr.sin_port)) {
	}

	IpAddress remoteAddr;
	unsigned short remotePort;

public:
	ClientSocket(const IpAddress &remoteAddr, unsigned short remotePort);
	~ClientSocket() throw() {}

	string getRemoteHostName() const;
	const IpAddress getRemoteAddr() const	{return remoteAddr;}
	unsigned short getRemotePort() const	{return remotePort;}

	friend class ServerSocket;
};

// =====================================================
//	class ServerSocket
// =====================================================

class ServerSocket : public Socket {
public:
	ServerSocket() : Socket(SOCKET_TYPE_SERVER) {};
	~ServerSocket() throw() {}

	void bind(unsigned short localPort);
	void listen(int connectionQueueSize = SOMAXCONN);
	ClientSocket *accept();
};

}}//end namespace

#endif
