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

#if 0

#include "pch.h"
#include "opengl.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "context_gl.h"
#include "gl_wrap.h"

#include "leak_dumper.h"


using namespace Shared::Platform;
using namespace std;

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class Globals
// =====================================================

bool isGlExtensionSupported(const char *extensionName){
    const char *s;
	GLint len;
	const GLubyte *extensionStr= glGetString(GL_EXTENSIONS);

	s= reinterpret_cast<const char *>(extensionStr);
	len= strlen(extensionName);

	while ((s = strstr (s, extensionName)) != NULL) {
		s+= len;

		if((*s == ' ') || (*s == '\0')) {
			return true;
		}
	}

	return false;
}

bool isGlVersionSupported(int major, int minor, int release){

	string stringVersion = getGlVersion();
	const char *strVersion = stringVersion.c_str();

	//major
	const char *majorTok= strVersion;
	int majorSupported= atoi(majorTok);

	if(majorSupported<major){
		return false;
	}
	else if(majorSupported>major){
		return true;
	}

	//minor
	int i=0;
	while(strVersion[i]!='.'){
		++i;
	}
	const char *minorTok= &strVersion[i]+1;
	int minorSupported= atoi(minorTok);

	if(minorSupported<minor){
		return false;
	}
	else if(minorSupported>minor){
		return true;
	}

	//release
	++i;
	while(strVersion[i]!='.'){
		++i;
	}
	const char *releaseTok= &strVersion[i]+1;

	if(atoi(releaseTok)<release){
		return false;
	}

	return true;
}

string getGlVersion(){
	return string(reinterpret_cast<const char *>(glGetString(GL_VERSION)));
}

string getGlRenderer(){
	return string(reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
}

string getGlVendor(){
	return string(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
}

string getGlExtensions(){
	return string(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
}

string getGlPlatformExtensions() {
	Context *c= GraphicsInterface::getInstance().getCurrentContext();
	return static_cast<ContextGl*>(c)->getPlatformContextGl()->getPlatformExtensions();
}

int getGlMaxLights(){
	int i;
	glGetIntegerv(GL_MAX_LIGHTS, &i);
	return i;
}

int getGlMaxTextureSize(){
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
	return i;
}

int getGlMaxTextureUnits(){
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &i);
	return i;
}

int getGlModelviewMatrixStackDepth(){
	int i;
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &i);
	return i;
}

int getGlProjectionMatrixStackDepth(){
	int i;
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &i);
	return i;
}

void checkGlExtension(const char *extensionName){
	if(!isGlExtensionSupported(extensionName)){
		throw runtime_error("OpenGL extension not supported: " + string(extensionName));
	}
}

}}}// end namespace
#endif