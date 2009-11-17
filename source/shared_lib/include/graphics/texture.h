// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_TEXTURE_H_
#define _SHARED_GRAPHICS_TEXTURE_H_

#include "types.h"
#include "pixmap.h"
#include "opengl.h"

#include <string>

using std::string;
using Shared::Platform::uint8;

namespace Shared{ namespace Graphics{

class TextureParams;

// =====================================================
//	class Texture
// =====================================================

class Texture{
protected:
	GLuint handle;	

public:
	static const int defaultSize;

	enum WrapMode{
		wmRepeat,
		wmClamp,
		wmClampToEdge
	};

	enum Filter{
		fBilinear,
		fTrilinear
	};

	enum Format{
		fAuto,
		fAlpha,
		fLuminance,
		fRgb,
		fRgba
	};

	GLuint getHandle() const	{return handle;}

protected:
	string path;
	bool mipmap;
	WrapMode wrapMode;
	bool pixmapInit;
	Format format;

	bool inited;

public:
	Texture();
	virtual ~Texture(){};
	
	bool getMipmap() const			{return mipmap;}
	WrapMode getWrapMode() const	{return wrapMode;}
	bool getPixmapInit() const		{return pixmapInit;}
	Format getFormat() const		{return format;}
	const string getPath() const	{return path;}

	void setMipmap(bool mipmap)			{this->mipmap= mipmap;}
	void setWrapMode(WrapMode wrapMode)	{this->wrapMode= wrapMode;}
	void setPixmapInit(bool pixmapInit)	{this->pixmapInit= pixmapInit;}
	void setFormat(Format format)		{this->format= format;}

	virtual void init(Filter filter= fBilinear, int maxAnisotropy= 1)=0;
	void end(){
		if(inited){
			assertGl();
			glDeleteTextures(1, &handle);
			assertGl();
		}
	}
};

// =====================================================
//	class Texture1D
// =====================================================

class Texture1D: public Texture{
protected:
	Pixmap1D pixmap;

public:
	void load(const string &path);

	Pixmap1D *getPixmap()				{return &pixmap;}
	const Pixmap1D *getPixmap() const	{return &pixmap;}
	virtual void init(Filter filter, int maxAnisotropy= 1);
};

// =====================================================
//	class Texture2D
// =====================================================

class Texture2D: public Texture{
protected:
	Pixmap2D pixmap;

public:
	void load(const string &path);

	Pixmap2D *getPixmap()				{return &pixmap;}
	const Pixmap2D *getPixmap() const	{return &pixmap;}
	virtual void init(Filter filter, int maxAnisotropy= 1);
};

// =====================================================
//	class Texture3D
// =====================================================

class Texture3D: public Texture{
protected:
	Pixmap3D pixmap;

public:
	void loadSlice(const string &path, int slice);

	Pixmap3D *getPixmap()				{return &pixmap;}
	const Pixmap3D *getPixmap() const	{return &pixmap;}
	virtual void init(Filter filter, int maxAnisotropy= 1);
};

// =====================================================
//	class TextureCube
// =====================================================

class TextureCube: public Texture{
protected:
	PixmapCube pixmap;

public:
	void loadFace(const string &path, int face);

	PixmapCube *getPixmap()				{return &pixmap;}
	const PixmapCube *getPixmap() const	{return &pixmap;}
	virtual void init(Filter filter, int maxAnisotropy= 1);
};

}}//end namespace

#endif
