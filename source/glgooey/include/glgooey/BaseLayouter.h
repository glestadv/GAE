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
#ifndef BASELAYOUTER__H
#define BASELAYOUTER__H

#include "mmgr/mmgr.h"

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class Window;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Abstract base class for all layout info classes
//!
//! If a layouter requires information per child window, there is an
//! addWindow method that takes an additional instance of a class derived
//! from LayoutInfo. For example, the ComplexGridLayouter uses instances
//! of ComplexGridCellInfo (which is derived from LayoutInfo) to provide
//! information such as row, column etc. on a per child window basis
class LayoutInfo
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    LayoutInfo() {}
    virtual ~LayoutInfo() {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! poor man's RTTI
    virtual std::string type() const = 0;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Abstract base class for all layout classes
class BaseLayouter
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor
    BaseLayouter() {}

    //! destructor
    virtual ~BaseLayouter() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! adds a window to the layouter
    virtual BaseLayouter& addWindow(Window* window) = 0;

    //! \brief adds a window to the layouter with additional layout info. If a
    //! derived class does not implement this, then it just calls
    //! addWindow with the \c window parameter and ignores the \c info
    //! parameter
    virtual BaseLayouter& addWindow(Window* window, const LayoutInfo& )
    {
        addWindow(window);
        return *this;
    }

    //! removes a window from the layouter
    virtual BaseLayouter& removeWindow(Window* window) = 0;

    //! get the layouter to arrange the windows
    virtual BaseLayouter& arrangeWithin(Window* parentWindow) = 0;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
