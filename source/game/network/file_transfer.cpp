// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "file_transfer.h"

#include <stdexcept>
#include <cassert>

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Game { namespace Net {

FileReceiver::FileReceiver(const NetworkMessageFileHeader &msg, const string &outdir) :
		name(msg.getName()),
		compressed(msg.isCompressed()),
		size(msg.getSize()),
		path(outdir + "/" + msg.getName()),
		out(path.c_str(), ios::binary | ios::out | ios::trunc),
		finished(false),
		nextseq(0) {
	if(out.bad()) {
		throw runtime_error("Failed to open new file for output: " + msg.getName());
	}
}

FileReceiver::~FileReceiver() {
}

/** 
 * Processes a file fragment message.
 * @return true when file download is complete.
 */
bool FileReceiver::processFragment(const NetworkMessageFileFragment &msg) {
    int zstatus;

	assert(!finished);
	if(finished) {
		throw runtime_error(string("Received file fragment after download of file ")
				+ name + " was already completed.");
	}

	if(!compressed) {
		out.write(msg.getData(), msg.getDataSize());
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
		return msg.isLast();
	}

	if(nextseq == 0){
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		z.avail_in = 0;
		z.next_in = Z_NULL;

		if(inflateInit(&z) != Z_OK) {
			throw runtime_error(string("Failed to initialize zstream: ") + z.msg);
		}
	}

	if(nextseq++ != msg.getSeq()) {
		throw runtime_error("File fragments arrived out of sequence, which isn't supposed to "
				"happen with stream sockets.  Did somebody change the socket implementation to "
				"datagrams? (NOTE: This is known to happen when running a windows build on wine.)");
	}

	z.avail_in = msg.getDataSize();
	z.next_in = (Bytef*)msg.getData();
	do {
		z.avail_out = sizeof(buf);
		z.next_out = (Bytef*)buf;
		zstatus = inflate(&z, Z_NO_FLUSH);
		assert(zstatus != Z_STREAM_ERROR);	// state not clobbered
		switch (zstatus) {
		case Z_NEED_DICT:
			zstatus = Z_DATA_ERROR;
			// intentional fall-through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			throw runtime_error(string("error in zstream: ") + z.msg);
		}
		out.write(buf, sizeof(buf) - z.avail_out);
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
	} while (z.avail_out == 0);

	if(msg.isLast() && zstatus != Z_STREAM_END) {
		throw runtime_error("Unexpected end of zstream data.");
	}

	if(msg.isLast() || zstatus == Z_STREAM_END) {
		finished = true;
		inflateEnd(&z);
	}

	return msg.isLast();
};

}} // end namespace
