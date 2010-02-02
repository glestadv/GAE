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

// we need ntohll & htonll functions
#ifdef WORDS_BIGENDIAN
#	define ntohll(x) (x)
#	define htonll(x) (x)
#else
#	ifndef HAVE_BYTESWAP_H
		// note: _bswap_constant_64 from GNU C Library, copyright Free Software Foundation, Inc.
#		define _bswap_constant_64(x)				\
			 ((((x) & 0xff00000000000000ull) >> 56)	\
			| (((x) & 0x00ff000000000000ull) >> 40)	\
			| (((x) & 0x0000ff0000000000ull) >> 24)	\
			| (((x) & 0x000000ff00000000ull) >> 8)	\
			| (((x) & 0x00000000ff000000ull) << 8)	\
			| (((x) & 0x0000000000ff0000ull) << 24)	\
			| (((x) & 0x000000000000ff00ull) << 40)	\
			| (((x) & 0x00000000000000ffull) << 56))
		inline int64 bswap_64(int64 x)	 {int64 v = (x); return _bswap_constant_64(v);}
#	endif
#	define ntohll(x) bswap_64(x)
#	define htonll(x) bswap_64(x)
#endif

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



/**
 * Implementation Notes: The reason for the *reinterpret_cast<uint16_t *>(&data) type of operations
 * you see in these functions is to cause a cast of the data type without any integeral promotion
 * or other changes to the underlying data as a result of differences in signedness.
 */
class NetworkDataBuffer : public SimpleDataBuffer {
public:
	NetworkDataBuffer() : SimpleDataBuffer(0x400) {}
	NetworkDataBuffer(size_t initial) : SimpleDataBuffer(initial) {}
	virtual ~NetworkDataBuffer() {}

	void peek(void *dest, size_t count) const	{SimpleDataBuffer::peek(dest, count);}
	void read(void *dest, size_t count)			{SimpleDataBuffer::read(dest, count);}
	void write(const void *src, size_t count)	{SimpleDataBuffer::write(src, count);}

	void read(int8 &data)	{read(&data, 1);}
	void read(uint8 &data)	{read(&data, 1);}
	void read(int16 &data)	{uint16_t a; read(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void read(uint16 &data)	{uint16_t a; read(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void read(int32 &data)	{uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(uint32 &data)	{uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(float32 &data){uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(int64 &data)	{uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void read(uint64 &data)	{uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void read(float64 &data){uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}

	void peek(int8 &data) const		{peek(&data, 1);}
	void peek(uint8 &data) const	{peek(&data, 1);}
	void peek(int16 &data) const	{uint16_t a; peek(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void peek(uint16 &data) const	{uint16_t a; peek(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void peek(int32 &data) const	{uint32_t a; peek(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(uint32 &data) const	{uint32_t a; peek(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(float32 &data) const	{uint32_t a; peek(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(int64 &data) const	{uint64_t a; peek(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void peek(uint64 &data) const	{uint64_t a; peek(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void peek(float64 &data) const	{uint64_t a; peek(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}

	void write(int8 data)	{write(&data, 1);}
	void write(uint8 data)	{write(&data, 1);}
	void write(int16 data)	{uint16_t a = htons(*reinterpret_cast<uint16_t *>(&data)); write(&a, 2);}
	void write(uint16 data)	{uint16_t a = htons(*reinterpret_cast<uint16_t *>(&data)); write(&a, 2);}
	void write(int32 data)	{uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(uint32 data)	{uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(float32 data){uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(int64 data)	{uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}
	void write(uint64 data)	{uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}
	void write(float64 data){uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}
};

class NetworkWriteable {
public:
	virtual ~NetworkWriteable() {}
	virtual size_t getNetSize() const = 0;
	virtual size_t getMaxNetSize() const = 0;
	//virtual void read(NetworkDataBuffer &buf) = 0;
	virtual void write(NetworkDataBuffer &buf) const = 0;
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

protected:
	SOCKET sock;

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
