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

#ifndef _SHARED_PLATFORM_NETUTIL_H_
#define _SHARED_PLATFORM_NETUTIL_H_

#include <cassert>
#include <vector>

#ifdef HAVE_BYTESWAP_H
#	include <byteswap.h>
#endif

#ifdef USE_POSIX_SOCKETS
#	include <arpa/inet.h>
#elif defined(WIN32) || defined(WIN64)
#	include <winsock2.h>
#	define uint16_t uint16
#	define uint32_t uint32
#	define uint64_t uint64
#endif

#include "types.h"
#include "simple_data_buffer.h"

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

	void peek(void *dest, size_t count, size_t skip = 0) const	{SimpleDataBuffer::peek(dest, count, skip);}
	void read(void *dest, size_t count)							{SimpleDataBuffer::read(dest, count);}
	void write(const void *src, size_t count)					{SimpleDataBuffer::write(src, count);}

	void read(int8 &data)	{read(&data, 1);}
	void read(uint8 &data)	{read(&data, 1);}
	void read(bool &data)	{char a;  read(&a, 1); data = static_cast<bool>(a);}
	void read(int16 &data)	{uint16_t a; read(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void read(uint16 &data)	{uint16_t a; read(&a, 2); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void read(int32 &data)	{uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(uint32 &data)	{uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(float32 &data){uint32_t a; read(&a, 4); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void read(int64 &data)	{uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void read(uint64 &data)	{uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void read(float64 &data){uint64_t a; read(&a, 8); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}

	void peek(int8 &data, size_t skip = 0) const	{peek(&data, 1, skip);}
	void peek(uint8 &data, size_t skip = 0) const	{peek(&data, 1, skip);}
	void peek(bool &data, size_t skip = 0) const	{char a;  peek(&a, 1, skip); data = static_cast<bool>(a);}
	void peek(int16 &data, size_t skip = 0) const	{uint16_t a; peek(&a, 2, skip); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void peek(uint16 &data, size_t skip = 0) const	{uint16_t a; peek(&a, 2, skip); *reinterpret_cast<uint16_t *>(&data) = ntohs(a);}
	void peek(int32 &data, size_t skip = 0) const	{uint32_t a; peek(&a, 4, skip); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(uint32 &data, size_t skip = 0) const	{uint32_t a; peek(&a, 4, skip); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(float32 &data, size_t skip = 0) const	{uint32_t a; peek(&a, 4, skip); *reinterpret_cast<uint32_t *>(&data) = ntohl(a);}
	void peek(int64 &data, size_t skip = 0) const	{uint64_t a; peek(&a, 8, skip); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void peek(uint64 &data, size_t skip = 0) const	{uint64_t a; peek(&a, 8, skip); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}
	void peek(float64 &data, size_t skip = 0) const	{uint64_t a; peek(&a, 8, skip); *reinterpret_cast<uint64_t *>(&data) = ntohll(a);}

	void write(int8 data)	{write(&data, 1);}
	void write(uint8 data)	{write(&data, 1);}
	void write(bool data)	{char a = data ? 1 : 0; write(&a, 1);}
	void write(int16 data)	{uint16_t a = htons(*reinterpret_cast<uint16_t *>(&data)); write(&a, 2);}
	void write(uint16 data)	{uint16_t a = htons(*reinterpret_cast<uint16_t *>(&data)); write(&a, 2);}
	void write(int32 data)	{uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(uint32 data)	{uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(float32 data){uint32_t a = htonl(*reinterpret_cast<uint32_t *>(&data)); write(&a, 4);}
	void write(int64 data)	{uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}
	void write(uint64 data)	{uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}
	void write(float64 data){uint64_t a = htonll(*reinterpret_cast<uint64_t *>(&data)); write(&a, 8);}

};

class NetSerializable {
public:
	virtual ~NetSerializable() {}
	virtual size_t getNetSize() const = 0;
	virtual size_t getMaxNetSize() const = 0;
	virtual void read(NetworkDataBuffer &buf) = 0;
	virtual void write(NetworkDataBuffer &buf) const = 0;
};

}} // end namespace

#endif // _SHARED_PLATFORM_NETUTIL_H_
