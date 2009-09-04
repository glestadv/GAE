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
#ifndef __GOOEY_FLOW_LAYOUTER__H
#define __GOOEY_FLOW_LAYOUTER__H

#include "mmgr/mmgr.h"

#include "glgooey/BaseLayouter.h"


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
//! \brief Arranges the windows in rows with given spacings
//!
//! A panel using the box layouter will arrange its child components in either a row or a
//! column depending on the justification. The box layouter can be forced to respect the sizes
//! of child components in which case child components may end up outside the panel. Alternatively,
//! the box layouter will resize the child components to fit within the available space.
class BoxLayouter : public BaseLayouter
{
    // ---------------------------------------------------------------------------------------------
    //  Internal Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the possible alignment types
    enum Justification { horizontal = 0, vertical };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief default constructor causes left aligned layouts with a
    //! horizontal and vertical spacing of 4 pixels
    explicit BoxLayouter(Justification aJustification = horizontal);

    //! This constructor requires all justification and spacing info and whether to respect child size
    BoxLayouter(Justification aJustification, float aSpacing, bool respectChildSizes);

    //! destructor
    virtual ~BoxLayouter() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the justification used by this layouter
    Justification justification() const { return justification_; }

    //! Sets the justification used by this layouter
    BoxLayouter& setJustification(Justification aJustification)
    {
        justification_ = aJustification;
        return *this;
    }

    //! Returns the spacing between child windows
    float spacing() const { return spacing_; }

    //! Sets the spacing used by this layouter
    BoxLayouter& setSpacing(float aSpacing)
    {
        spacing_ = aSpacing;
        return *this;
    }

    //! Returns true iff child sizes are respected - i.e. the layouter does not resize its children
    bool isChildSizeRespected() const { return isChildSizeRespected_; }

    //! \brief Call this to force the layouter to respect child size - After calling this, the
    //! layouter will not alter the sizes of child windows
    BoxLayouter& respectChildSize() { isChildSizeRespected_ = true; return *this; }

    //! \brief Call this to force the layouter not to respect child size - After calling this, the
    //! layouter will alter the sizes of child windows to ensure that they fit in the available space
    BoxLayouter& doNotRespectChildSize() { isChildSizeRespected_ = false; return *this; }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! adds a window to the layouter
    virtual BaseLayouter& addWindow(Window* window);

    //! removes a window from the layouter
    virtual BaseLayouter& removeWindow(Window* window);

    //! get the layouter to arrange the windows
    virtual BaseLayouter& arrangeWithin(Window* parentWindow);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    Justification justification_;
    float spacing_;
    bool  isChildSizeRespected_;
    std::list<Window*> windows_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
