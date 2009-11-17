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

#include "pch.h"
#include "texture.h"
#include "opengl.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

using namespace Gl;
using namespace Platform;

GLint toWrapModeGl(Texture::WrapMode wrapMode){
	switch(wrapMode){
	case Texture::wmClamp:
		return GL_CLAMP;
	case Texture::wmRepeat:
		return GL_REPEAT;
	case Texture::wmClampToEdge:
		return GL_CLAMP_TO_EDGE;
	default:
		assert(false);
		return GL_CLAMP;
	}
}

GLint toFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			assert(false);
			return GL_RGBA;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE;
	case Texture::fAlpha:
		return GL_ALPHA;
	case Texture::fRgb:
		return GL_RGB;
	case Texture::fRgba:
		return GL_RGBA;
	default:
		assert(false);
		return GL_RGB;
	}
}

GLint toInternalFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE8;
		case 3:
			return GL_RGB8;
		case 4:
			return GL_RGBA8;
		default:
			assert(false);
			return GL_RGBA8;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE8;
	case Texture::fAlpha:
		return GL_ALPHA8;
	case Texture::fRgb:
		return GL_RGB8;
	case Texture::fRgba:
		return GL_RGBA8;
	default:
		assert(false);
		return GL_RGB8;
	}
}


// =====================================================
//	class Texture
// =====================================================

const int Texture::defaultSize= 256;

Texture::Texture(){
	mipmap= true;
	pixmapInit= true;
	wrapMode= wmRepeat;
	format= fAuto;

	inited= false;
}

// =====================================================
//	class Texture1D
// =====================================================

void Texture1D::load(const string &path){
	this->path= path;
	pixmap.load(path);
}

void Texture1D::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_1D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")){
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int error= gluBuild1DMipmaps(
				GL_TEXTURE_1D, glInternalFormat, pixmap.getW(),
				glFormat, GL_UNSIGNED_BYTE, pixels);

			if(error!=0){
				throw runtime_error("Error building texture 1D mipmaps");
			}
		}
		else{
			//build single texture
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage1D(
				GL_TEXTURE_1D, 0, glInternalFormat, pixmap.getW(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();
			if(error!=GL_NO_ERROR){
				throw runtime_error("Error creating texture 1D");
			}
		}
		inited= true;
	}

	assertGl();
}
// =====================================================
//	class Texture2D
// =====================================================

void Texture2D::load(const string &path){
	this->path= path;
	pixmap.load(path);
}

void Texture2D::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int error= gluBuild2DMipmaps(
				GL_TEXTURE_2D, glInternalFormat,
				pixmap.getW(), pixmap.getH(),
				glFormat, GL_UNSIGNED_BYTE, pixels);

			if(error!=0){
				throw runtime_error("Error building texture 2D mipmaps");
			}
		}
		else{
			//build single texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(
				GL_TEXTURE_2D, 0, glInternalFormat,
				pixmap.getW(), pixmap.getH(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();
			if(error!=GL_NO_ERROR){
				throw runtime_error("Error creating texture 2D");
			}
		}
		inited= true;
	}

	assertGl();
}
// =====================================================
//	class Texture3D
// =====================================================

void Texture3D::loadSlice(const string &path, int slice){
	this->path= path;
	pixmap.loadSlice(path, slice);
}

void Texture3D::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_3D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);

		//build single texture
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage3D(
			GL_TEXTURE_3D, 0, glInternalFormat,
			pixmap.getW(), pixmap.getH(), pixmap.getD(),
			0, glFormat, GL_UNSIGNED_BYTE, pixels);

		GLint error= glGetError();
		if(error!=GL_NO_ERROR){
			throw runtime_error("Error creating texture 3D");
		}
		inited= true;
	}

	assertGl();
}

// =====================================================
//	class TextureCube
// =====================================================

void TextureCube::loadFace(const string &path, int face){
	this->path= path;
	pixmap.loadFace(path, face);
}

void TextureCube::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

		//wrap
		GLint wrap= toWrapModeGl(wrapMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);

		//filter
		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		for(int i=0; i<6; ++i){
			//params
			const Pixmap2D *currentPixmap= pixmap.getFace(i);

			GLint glFormat= toFormatGl(format, currentPixmap->getComponents());
			GLint glInternalFormat= toInternalFormatGl(format, currentPixmap->getComponents());

			//pixel init var
			const uint8* pixels= pixmapInit? currentPixmap->getPixels(): NULL;
			GLenum target= GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			if(mipmap){
				int error= gluBuild2DMipmaps(
					target, glInternalFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					glFormat, GL_UNSIGNED_BYTE, pixels);

				if(error!=0){
					throw runtime_error("Error building texture cube mipmaps");
				}
			}
			else{
				glTexImage2D(
					target, 0, glInternalFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					0, glFormat, GL_UNSIGNED_BYTE, pixels);
			}

			if(glGetError()!=GL_NO_ERROR){
				throw runtime_error("Error creating texture cube");
			}
		}
		inited= true;

	}

	assertGl();
}

}}//end namespace
