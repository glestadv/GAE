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
#ifndef RENDERDESC__H
#define RENDERDESC__H

#include "mmgr/mmgr.h"

#include "glgooey/WindowManager.h"

// *************************************************************************************************
//  Forward Declarations
namespace Gooey
{
    namespace Core
    {
        class Rectangle;
    }
}
// *************************************************************************************************


// *************************************************************************************************
namespace Gooey
{

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Base class for all elements that are capable of being rendered
class RenderDesc
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    RenderDesc() {}

    //! Destructor
    virtual ~RenderDesc() {}


    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const = 0;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive) {}
};

// *************************************************************************************************
} // namespace Gooey

#endif
