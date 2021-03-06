// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

#include <string>

using std::string;
using Shared::Platform::uint8;

namespace Shared{ namespace Graphics{

class TextureParams;

// =====================================================
//	class Texture
// =====================================================

class Texture{
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

	virtual void init(Filter filter = fBilinear, int maxAnisotropy = 1) = 0;
	virtual void deletePixmap() = 0;
	virtual void end() = 0;
};

// =====================================================
//	class Texture1D
// =====================================================

class Texture1D: public Texture{
protected:
	Pixmap1D *pixmap;

public:
	Texture1D() : pixmap(new Pixmap1D()) {}
	~Texture1D() { delete pixmap; }
	void load(const string &path);

	Pixmap1D *getPixmap()				{return pixmap;}
	const Pixmap1D *getPixmap() const	{return pixmap;}
};

// =====================================================
//	class Texture2D
// =====================================================

class Texture2D: public Texture{
protected:
	Pixmap2D *pixmap;

public:
	static Texture2D *defaultTexture;

public:
	Texture2D() : pixmap(new Pixmap2D()) { }
	~Texture2D() { delete pixmap; }
	void load(const string &path);

	Pixmap2D *getPixmap()				{return pixmap;}
	void setPixmap(Pixmap2D *pm);
	void setPixmap(const Pixmap2D *pm) { setPixmap(const_cast<Pixmap2D*>(pm)); }
	const Pixmap2D *getPixmap() const	{return pixmap;}
};

// =====================================================
//	class Texture3D
// =====================================================

class Texture3D: public Texture{
protected:
	Pixmap3D *pixmap;

public:
	Texture3D() : pixmap(new Pixmap3D()) { }
	~Texture3D() { delete pixmap; }
	void loadSlice(const string &path, int slice);

	Pixmap3D *getPixmap()				{return pixmap;}
	const Pixmap3D *getPixmap() const	{return pixmap;}
};

// =====================================================
//	class TextureCube
// =====================================================

class TextureCube: public Texture{
protected:
	PixmapCube *pixmap;

public:
	TextureCube() : pixmap(new PixmapCube()) { }
	~TextureCube() { delete pixmap; }
	void loadFace(const string &path, int face);

	PixmapCube *getPixmap()				{return pixmap;}
	const PixmapCube *getPixmap() const	{return pixmap;}
};

}}//end namespace

#endif
