// __________________________________________________________________________________________________
//    GLGooey Graphical User Interface for OpenGL
//    Copyright (c) 2004 Niel Waldren
//
// This software is provided 'as-is', without any express or implied warranty. In no event will
// the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not claim that you
//        wrote the original software. If you use this software in a product, an acknowledgment
//        in the product documentation would be appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not be misrepresented
//        as being the original software.
//
//     3. This notice may not be removed or altered from any source distribution.
//
// __________________________________________________________________________________________________
#ifndef EXCEPTION__H
#define EXCEPTION__H

#include "mmgr/mmgr.h"

#include "glgooey/core/Utils.h"

// *************************************************************************************************
namespace Gooey
{

// *************************************************************************************************
namespace Core
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Gooey StandardExceptions
//!
//! GLGooey uses exceptions for error handling - all exceptions are instances of
//! this class. All exceptions are thrown using the THROW macro defined below
class StandardException : public std::exception
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Constructor requires a message which requested from the message method
    StandardException( const std::string& aMessage ) : what_(aMessage) {}

    //! destructor
    virtual ~StandardException() throw() {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the text describing the exception
    virtual const char* what() const throw() { return what_.c_str(); }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    std::string  what_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Core


// *************************************************************************************************
} // namespace Gooey


//! Gooey always uses this to throw exceptions
#define THROW(r) throw Gooey::Core::StandardException(r + std::string("filename: ") + std::string(__FILE__) + std::string("   line: ") + Gooey::Core::toString(__LINE__))

//! Asserts a boolean expression
#ifndef ASSERT
#define ASSERT(b) if(!(b)) THROW(std::string("Assertion <") + #b + std::string("> failed"))
#endif


#endif
