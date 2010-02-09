// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_SOCKET_H_
#define _SHARED_PLATFORM_SOCKET_H_

#include <string>
#include <cassert>

#ifdef HAVE_BYTESWAP_H
#	include <byteswap.h>
#endif

#ifdef USE_POSIX_SOCKETS
#	include <unistd.h>
#	include <errno.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <fcntl.h>

#	include "types.h"
#	include "util.h"

#elif defined(WIN32) || defined(WIN64)
#	include <winsock.h>

#	include "types.h"
#	include "util.h"

#	define uint16_t uint16
#	define uint32_t uint32
#	define uint64_t uint64
#endif

using std::string;
using Shared::Util::SimpleDataBuffer;

namespace Shared { namespace Platform {

// =====================================================
//	class IP
// =====================================================

class Ip {
private:
	unsigned char bytes[4];

public:
	Ip();
	Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3);
	Ip(const string& ipString);

	unsigned char getByte(int byteIndex)	{return bytes[byteIndex];}
	string getString() const;
};

class SocketException : public runtime_error {
public:
	SocketException(const string &msg) : runtime_error(msg){}
};

// =====================================================
//	class Socket
// =====================================================

class Socket {
#if defined(WIN32) || defined(WIN64)
	class LibraryManager {
	public:
		LibraryManager();
		~LibraryManager();
	};

	static LibraryManager libraryManager;
#endif

private:
	typedef char* char_ptr;

	class CircularBuffer {
		static const int buffer_size = 16 * 1024;

		char_ptr buffer;
		size_t tail, head;
		bool full;

	public:
		CircularBuffer() : head(0), tail(0), buffer(0), full(false) {
			buffer = new char[buffer_size]; 
		}
		~CircularBuffer() { delete [] buffer; }

		char_ptr getWritePos() const { return buffer + head; }
		void operator+=(size_t b);
		void operator-=(size_t b);
		size_t bytesAvailable() const;
		bool peekBytes(void *dst, size_t n);
		bool readBytes(void *dst, int n);
		int getMaxWrite(bool &limit) const;
		size_t getFreeBytes() const;
	};

protected:
	SOCKET sock;
	CircularBuffer buffer;

public:
	Socket(SOCKET sock);
	Socket();
	~Socket();

	int getDataToRead();
	int send(const void *data, int dataSize);
	int receive(void *data, int dataSize);
	int peek(void *data, int dataSize);

	void setBlock(bool block);
	bool isReadable();
	bool isWritable();
	bool isConnected();

	string getHostName() const;
	string getIp() const;

protected:
	void handleError(const char *caller) const;
	void readAll();
};

// =====================================================
//	class ClientSocket
// =====================================================

class ClientSocket: public Socket {
public:
	void connect(const Ip &ip, int port);
};

// =====================================================
//	class ServerSocket
// =====================================================

class ServerSocket: public Socket {
public:
	void bind(int port);
	void listen(int connectionQueueSize= SOMAXCONN);
	Socket *accept();
};

}}//end namespace

#endif
