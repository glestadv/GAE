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
#ifndef COMPLEXGRIDLAYOUTER__H
#define COMPLEXGRIDLAYOUTER__H

#include "mmgr/mmgr.h"

#include "glgooey/BaseLayouter.h"

#include "glgooey/core/Vector2.h"



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Provides information pertaining to an individual cell in a complex grid
//! \see ComplexGridLayouter
class ComplexGridCellInfo : public LayoutInfo
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    ComplexGridCellInfo();
    virtual ~ComplexGridCellInfo() {}

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
public:
    //! the number of horizontal cells the associated child window spans
    unsigned int horizontalSpan;

    //! the number of vertical cells the associated child window spans
    unsigned int verticalSpan;

    //! the index of the left-most column filled by the associated child window
    unsigned int columnIndex;

    //! the index of the top-most row filled by the associated child window
    unsigned int rowIndex;

    //! the amount of space to be left on each side of the child window
    float horizontalSpacing;

    //! the amount of space to be left above and below the child window
    float verticalSpacing;

    //! poor man's RTTI
    virtual std::string type() const { return type_; }

    //! \brief setting the row or column index to this value will cause the
    //! associated window to be placed in the next available cell, to the
    //! right or below the previous window.
    static const unsigned int relativeToPrevious;

private:
    //! poor man's RTTI
    static std::string type_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Arranges the windows in a grid with a given number of rows and columns
//!
//! All the windows in the ComplexGridLayouter are arranged in a grid which is
//! defined by the the information provided in the ComplexGridCellInfo class.
//! \see ComplexGridCellInfo
// ***************************************************************************
class ComplexGridLayouter : public BaseLayouter
{
    // ---------------------------------------------------------------------------------------------
    //  Internal Data Types
    // ---------------------------------------------------------------------------------------------
private:
    struct Cell
    {
        Window* window;
        ComplexGridCellInfo info;

        Cell(Window* aWindow, const ComplexGridCellInfo& cellInfo)
        {
            window = aWindow;
            info = cellInfo;
        }
    };

    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor
    explicit ComplexGridLayouter(const Core::Vector2& aBorderSize = Core::Vector2(2.0f, 2.0f)) :
        border_Size(aBorderSize)
    {}

    //! destructor
    virtual ~ComplexGridLayouter() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Sets the widths of the first \c n columns.
    //!
    //!  Sets the widths of the first \c n columns. Where \c n is the number
    //! of items in the passed in vector. If a window is added to a cell for
    //! which no column width has been specified, the remaining horizontal
    //! space is divided evenly by the number of columns whose widths have
    //! not been specified. The same goes for columns whose width has been set
    //! to 0. This means that one can specify widths 30, 0, 30 for a layout
    //! with three columns and the central column will expand to fill in the
    //! space between the left and right columns.
    ComplexGridLayouter& setColumnWidths(const std::vector<float>& widths);

    //! \brief Sets the heights of the first \c n rows.
    //!
    //!  Sets the heights of the first \c n rows. Where \c n is the number
    //! of items in the passed in vector. If a window is added to a cell for
    //! which no row height has been specified, the remaining vertical
    //! space is divided evenly by the number of columns whose widths have
    //! not been specified. The same goes for rows whose height has been set
    //! to 0. This means that one can specify heights 30, 0, 30 for a layout
    //! with three rows and the central row will expand to fill in the space
    //! between the top and bottom rows.
    ComplexGridLayouter& setRowHeights(const std::vector<float>& heights);

    //! \brief returns the width of the column with the passed in index or 0 if
    //! no value has been specified
    float columnWidth(int index) const;

    //! \brief returns the height of the row with the passed in index or 0 if
    //! no value has been specified
    float rowHeight(int index) const;



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! adds a window to the layouter with default cell info
    virtual BaseLayouter& addWindow(Window* window);

    //! adds a window to the layouter with user-defined cell info
    virtual BaseLayouter& addWindow(Window* window, const LayoutInfo& info);

    //! removes a window from the layouter
    virtual BaseLayouter& removeWindow(Window* window);

    //! get the layouter to arrange the windows
    virtual BaseLayouter& arrangeWithin(Window* parentWindow);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    std::list<Cell> _cells;
    Core::Vector2 border_Size;

    typedef std::vector<float>::iterator SizeIterator;
    std::vector<float> columnWidths_;
    std::vector<float> _rowHeights;


    struct Block
    {
        unsigned int row;
        unsigned int col;
        unsigned int hSpan;
        unsigned int vSpan;
        float hSpace;
        float vSpace;
        bool windowStart;
        Window* window;
    };

    void calculateBlocks();
    void calculateColWidths(std::vector<float>& colWidths, float width);
    void calculateRowHeights(std::vector<float>& rowHeights, float height);
    void addBlockToMap(const Block& aBlock)
    {
        _blockMap[aBlock.row*256 + aBlock.col] = aBlock;
    }

    unsigned int _numCols;
    unsigned int _numRows;
    std::map<unsigned int, Block> _blockMap;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
