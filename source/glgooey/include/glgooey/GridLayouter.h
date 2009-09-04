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
#ifndef GRIDLAYOUTER__H
#define GRIDLAYOUTER__H


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
//! \brief Arranges the windows in a grid with a given number of rows and columns
//!
//! All the windows in the GridLayouter are arranged in a uniform grid with
//! the given number of rows and columns and the given spacings. The available
//! space int the parent window is then split evenly amongst all windows. If
//! the number of rows or columns is set to 0, then its actual value will be
//! calculated from the number of windows. The order of arrangement is left to
//! right, top to bottom.
class GridLayouter : public BaseLayouter
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief default constructor
    //!
    //! a grid with one column and as many rows as there
    //! are windows to be arranged and a horizontal and vertical spacing of
    //! 4 pixels
    GridLayouter();

    //! \brief construction via number of rows and columns
    //!
    //! This constructor requires the number of rows and columns. If
    //! the number of rows or columns is set to 0, then its actual value
    //! will be calculated from the number of windows. The horizontal and
    //! vertical spacing is 4 pixels
    GridLayouter(size_t numRows, size_t numColumns);

    //! \brief construction via number of rows and columns and spacing information
    //!
    //! This constructor requires the number of rows and columns. If
    //! the number of rows or columns is set to 0, then its actual value
    //! will be calculated from the number of windows. The horizontal and
    //! vertical spacing values are also required
    GridLayouter(size_t numRows, size_t numColumns,
        size_t aHorizontalSpacing, size_t aVerticalSpacing);

    //! destructor
    virtual ~GridLayouter() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Returns the number of rows used by this layouter. If
    //! this value is set to 0, then its actual value will be
    //! calculated from the number of windows.
    size_t numberOfRows() const { return numberOfRows_; }

    //! sets the number of rows used by this layouter
    GridLayouter& setNumberOfRows(size_t numRows)
    {
        numberOfRows_ = numRows;
        return *this;
    }

    //! \brief Returns the number of columns used by this layouter. If
    //! this value is set to 0, then its actual value will be
    //! calculated from the number of windows.
    size_t numberOfColumns() const { return numberOfColumns_; }

    //! sets the number of columns used by this layouter
    GridLayouter& setNumberOfColumns(size_t numColumns)
    {
        numberOfColumns_ = numColumns;
        return *this;
    }

    //! returns the horizontal spacing used by this layouter
    size_t horizontalSpacing() const { return horizontalSpacing_; }

    //! sets the horizontal spacing used by this layouter
    GridLayouter& setHorizontalSpacing(size_t anHorizontalSpacing)
    {
        horizontalSpacing_ = anHorizontalSpacing;
        return *this;
    }

    //! returns the vertical spacing used by this layouter
    size_t verticalSpacing() const { return verticalSpacing_; }

    //! sets the vertical spacing used by this layouter
    GridLayouter& setVerticalSpacing(size_t aVerticalSpacing)
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
    size_t numberOfRows_;
    size_t numberOfColumns_;
    size_t horizontalSpacing_;
    size_t verticalSpacing_;
    std::list<Window*> windows_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
