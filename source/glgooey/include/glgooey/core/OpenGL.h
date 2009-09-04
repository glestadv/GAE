// **************************************************************************************************
//  !! NOTE !!
//  The contents of this file are due to the work of Henry Maddocks. The code contained within
//  is from the FTGL library. The original license notice follows:
// **************************************************************************************************
// Copyright (C) 2001-3 Henry Maddocks
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// __________________________________________________________________________________________________
#ifndef OPENGL__H
#define OPENGL__H
#ifndef WIN32

    // non windows, doesn't require nonesense as seen below :-)
    #ifndef __gl_h_
        #ifdef __APPLE_CC__
            #include <OpenGL/gl.h>
            #include <OpenGL/glu.h>
        #else
        #include <GL/gl.h>
        #include <GL/glu.h>
        #endif

    #endif

    // required for compatibility with glext.h style function definitions of
    // OpenGL extensions, such as in src/osg/Point.cpp.
    #ifndef APIENTRY
        #define APIENTRY
    #endif

#else   //  WIN32

    // Under windows avoid including <windows.h>
    // to avoid name space pollution, but Win32's <GL/gl.h>
    // needs APIENTRY and WINGDIAPI defined properly.
    // F
    # if 0
    #  define  WIN32_LEAN_AND_MEAN
    #  include <windows.h>
    # else
    // XXX This is from Win32's <windef.h>
    #  ifndef APIENTRY
    #   define GLUT_APIENTRY_DEFINED
    #   if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
    #    define APIENTRY    __stdcall
    #   else
    #    define APIENTRY
    #   endif
    #  endif
    // XXX This is from Win32's <winnt.h>
    #  ifndef CALLBACK
    #   if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
    #    define CALLBACK __stdcall
    #   else
    #    define CALLBACK
    #   endif
    #  endif
    // XXX This is from Win32's <wingdi.h> and <winnt.h>
    #  ifndef WINGDIAPI
    #   define GLUT_WINGDIAPI_DEFINED
    #   define WINGDIAPI __declspec(dllimport)
    #  endif
    // XXX This is from Win32's <ctype.h>
    #  ifndef _WCHAR_T_DEFINED
    typedef unsigned short wchar_t;
    #   define _WCHAR_T_DEFINED
    #  endif
    # endif

    #ifndef __gl_h_
        #include <GL/gl.h>
        #include <GL/glu.h>
    #endif

#endif  //  WIN32

#ifndef GL_EXT_texture_object
    #define GL_TEXTURE_PRIORITY_EXT           0x8066
    #define GL_TEXTURE_RESIDENT_EXT           0x8067
    #define GL_TEXTURE_1D_BINDING_EXT         0x8068
    #define GL_TEXTURE_2D_BINDING_EXT         0x8069
    #define GL_TEXTURE_3D_BINDING_EXT         0x806A
#endif

#endif
