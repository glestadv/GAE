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
#ifndef FLOWLAYOUTER__H
#define FLOWLAYOUTER__H

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
//! All windows within the FlowLayouter are arranged in rows, one after the
//! other separated by a horizontal spacing, and each row separated by a
//! vertical spacing. The FlowLayouter can arrange the windows left, right
//! or center aligned with respect to the parent window.
class FlowLayouter : public BaseLayouter
{
    // ---------------------------------------------------------------------------------------------
    //  Internal Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the possible alignment types
    enum Alignment { left = 0, right, center };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief default constructor causes left aligned layouts with a
    //! horizontal and vertical spacing of 4 pixels
    explicit FlowLayouter(Alignment anAlignment = left);

    //! This constructor requires all alignment and spacing info
    FlowLayouter(Alignment anAlignment, float aHorizontalSpacing, float aVerticalSpacing);

    //! destructor
    virtual ~FlowLayouter() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the alignment used by this layouter
    Alignment alignment() const { return alignment_; }

    //! sets the alignment used by this layouter
    FlowLayouter& setAlignment(Alignment anAlignment)
    {
        alignment_ = anAlignment;
        return *this;
    }

    //! returns the horizontal spacing used by this layouter
    float horizontalSpacing() const { return horizontalSpacing_; }

    //! sets the horizontal spacing used by this layouter
    FlowLayouter& setHorizontalSpacing(float anHorizontalSpacing)
    {
        horizontalSpacing_ = anHorizontalSpacing;
        return *this;
    }

    //! returns the vertical spacing used by this layouter
    float verticalSpacing() const { return verticalSpacing_; }

    //! sets the vertical spacing used by this layouter
    FlowLayouter& setVerticalSpacing(float aVerticalSpacing)
    {
        verticalSpacing_ = aVerticalSpacing;
        return *this;
    }



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
    Alignment alignment_;
    float horizontalSpacing_;
    float verticalSpacing_;
    std::list<Window*> windows_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
