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

#include "pch.h"
#include "shader.h"

#include "FSFactory.hpp"

#include <stdexcept>
#include <fstream>

//#include "leak_dumper.h"

using namespace std;
using namespace Shared::PhysFS;

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderSource
// =====================================================

void ShaderSource::load(const string &path){
	pathInfo+= path + " ";

	// load the whole file into memory
	FileOps *f = FSFactory::getInstance()->getFileOps();
	try {
		f->openRead(path.c_str());
		int length = f->fileSize();
		this->code = new char[length+1];
		f->read(this->code, length, 1);
		this->code[length] = '\0'; // null terminator
	} catch (exception &e) {
		cerr << e.what() << endl;
		this->code = new char[1];
		this->code[0] = '\0';
	}
	
	delete f;
}

DefaultShaderProgram::DefaultShaderProgram() {
	m_p = glCreateProgram();
}

DefaultShaderProgram::~DefaultShaderProgram() {
	glDetachShader(m_p, m_v);
	glDetachShader(m_p, m_f);

	glDeleteShader(m_v);
	glDeleteShader(m_f);
	glDeleteProgram(m_p);
}

static string lastShaderError;

void DefaultShaderProgram::show_info_log(
    GLuint object,
    PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
	GLint log_length;
	char *log;

	glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
	log = new char[log_length];
	glGet__InfoLog(object, log_length, NULL, log);
	cerr << log << endl;
	lastShaderError = log;
	delete[] log;
}

string DefaultShaderProgram::getError(const string path, bool vert) {
	show_info_log(m_v, glGetShaderiv, glGetShaderInfoLog);
	string res = string("Compile error in ") + (vert ? "vertex" : "fragment");
	res +=  " shader: " + path + "\n" + lastShaderError;
	return res;
}

void DefaultShaderProgram::load(const string &vertex, const string &fragment) {
	m_v = glCreateShader(GL_VERTEX_SHADER);
	m_f = glCreateShader(GL_FRAGMENT_SHADER);

	m_vertexShader.load(vertex);
	m_fragmentShader.load(fragment);

	const char *vs = NULL,*fs = NULL;
	vs = m_vertexShader.getCode();
	fs = m_fragmentShader.getCode();
	
	glShaderSource(m_v, 1, &vs,NULL);
	glShaderSource(m_f, 1, &fs,NULL);

	glCompileShader(m_v);
	GLint ok;
	glGetShaderiv(m_v, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		string msg = getError(vertex, true);
		glDeleteShader(m_v);
		throw runtime_error(msg);
	}

	glCompileShader(m_f);
	glGetShaderiv(m_f, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		string msg = getError(fragment, false);
		glDeleteShader(m_f);
		throw runtime_error(msg);
	}

	glAttachShader(m_p,m_f);
	glAttachShader(m_p,m_v);

	glLinkProgram(m_p);
	glGetProgramiv(m_p, GL_LINK_STATUS, &ok);
	if (!ok) {
		cerr << "Failed to link shader program:" << endl;
		show_info_log(m_p, glGetProgramiv, glGetProgramInfoLog);
		glDeleteProgram(m_p);
		return;
	}

}

void DefaultShaderProgram::begin() {
	glUseProgram(m_p);
}

void DefaultShaderProgram::end() {
	glUseProgram(0);
}

void DefaultShaderProgram::setUniform(const std::string &name, GLuint value) {
	int texture_location = glGetUniformLocation(m_p, name.c_str());
	glUniform1i(texture_location, value);
}

void DefaultShaderProgram::setUniform(const std::string &name, const Vec3f &value) {
	int uniform_loc = glGetUniformLocation(m_p, name.c_str());
	glUniform3fv(uniform_loc, 1, value.ptr());
}

}}//end namespace
