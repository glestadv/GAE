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

#ifndef _GAME_NET_FILETRANSFER_H_
#define _GAME_NET_FILETRANSFER_H_

#include <zlib.h>

#include "network_message.h"

#include "socket.h"

namespace Game { namespace Net {

// =====================================================
//	class FileReceiver
// =====================================================

class FileReceiver {
private:
	string name;	/**< The file name */
	bool compressed;/**< Rather or not the incoming file is compressed. */
	size_t size;	/**< Total size expected (uncompressed) */
	string path;	/**< The full path to the output file */
	ofstream out;	/**< The output file stream */
	z_stream z;		/**< The compression stream */
	char buf[4096];	/**< A working buffer */
	bool finished;	/**< True if this transfer is complete. */
	int nextseq;	/**< The next fragment needed. */

public:
	FileReceiver(const NetworkMessageFileHeader &msg, const string &outdir);
	~FileReceiver();

	bool processFragment(const NetworkMessageFileFragment &msg);
	bool isCompressed() const		{return compressed;}
	size_t getSize() const			{return size;}
	const string &getName()	const	{return name;}
	const string &getPath()	const	{return path;}
	bool isFinished() const			{return finished;}
	size_t getBytesWritten() 		{return out.tellp();}
	float getProgress() 			{return static_cast<float>(out.tellp()) / static_cast<float>(float(size));}
};

}} // end namespace

#endif
