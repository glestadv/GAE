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

#include <algorithm>
#include <numeric>

#include "glgooey/ComplexGridLayouter.h"

#include "glgooey/core/StandardException.h"
#include "glgooey/Window.h"

// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComplexGridLayouter&
ComplexGridLayouter::setColumnWidths(const std::vector<float>& widths)
{
    while(widths.size() > columnWidths_.size())
    {
        columnWidths_.push_back(0);
    }

    std::copy(widths.begin(), widths.end(), columnWidths_.begin());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComplexGridLayouter&
ComplexGridLayouter::setRowHeights(const std::vector<float>& heights)
{
    while(heights.size() > _rowHeights.size())
    {
        _rowHeights.push_back(0);
    }

    std::copy(heights.begin(), heights.end(), _rowHeights.begin());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ComplexGridLayouter::columnWidth(int index) const
{
    ASSERT(index > 0);
    return (index < int(columnWidths_.size())) ?  columnWidths_[index] : 0.0f;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ComplexGridLayouter::rowHeight(int index) const

{
    ASSERT(index > 0);
    return (index < int(_rowHeights.size())) ? _rowHeights[index] : 0.0f;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
ComplexGridLayouter::addWindow(Window* window)
{
    addWindow(window, ComplexGridCellInfo());
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
ComplexGridLayouter::addWindow(Window* window, const LayoutInfo& inf)
{
    ASSERT(inf.type() == "ComplexGridCellInfo");

    ComplexGridCellInfo info = (const ComplexGridCellInfo&)(inf);

    Cell cell(window, info);
    _cells.push_back(cell);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
ComplexGridLayouter::removeWindow(Window* window)
{
    std::list<Cell>::iterator it;
    for(it = _cells.begin(); it != _cells.end(); ++it)
    {
        if( (*it).window == window ) break;
    }
    if(it != _cells.end()) _cells.erase(it);

    return *this;

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
ComplexGridLayouter::arrangeWithin(Window* parentWindow)
{
    const float width  = parentWindow->size().x() - (2.0f * border_Size.x());
    const float height = parentWindow->size().y() - (2.0f * border_Size.y());

    const Core::Vector2 offset = parentWindow->position() + border_Size;

    const unsigned int numWindows = (unsigned int)(_cells.size());

    if(numWindows > 0)
    {
        calculateBlocks();

        std::vector<float> colWidths = columnWidths_;
        std::vector<float> rowHeights = _rowHeights;

        calculateColWidths(colWidths, width);
        calculateRowHeights(rowHeights, height);

        std::map<unsigned int, Block>::iterator it;
        for(it = _blockMap.begin(); it != _blockMap.end(); ++it)
        {
            const Block& block = (*it).second;

            if(block.windowStart)
            {
                SizeIterator xStartIt = colWidths.begin();
                std::advance(xStartIt, block.col);

                SizeIterator xEndIt = xStartIt;
                std::advance(xEndIt, block.hSpan);

                SizeIterator yStartIt = rowHeights.begin();
                std::advance(yStartIt, block.row);

                SizeIterator yEndIt = yStartIt;
                std::advance(yEndIt, block.vSpan);



                const float x = std::accumulate(colWidths.begin(),  xStartIt, 0.0f) + block.hSpace;
                const float y = std::accumulate(rowHeights.begin(), yStartIt, 0.0f) + block.vSpace;

                const float childWidth  = std::accumulate(xStartIt, xEndIt, 0.0f) - (2.0f * block.hSpace);
                const float childHeight = std::accumulate(yStartIt, yEndIt, 0.0f) - (2.0f * block.vSpace);

                block.window->setPosition(Core::Vector2(x, y) + offset);
                block.window->setSize(Core::Vector2(childWidth, childHeight));
            }
        }
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ComplexGridLayouter::calculateColWidths(std::vector<float>& colWidths, float width)
{
    float totalWidth = 0;
    float curWidth = 0;
    unsigned int numUnset = 0;
    for(SizeIterator it = colWidths.begin(); it != colWidths.end(); ++it)
    {
        float& w = *it;
        if(w == 0.0f)
            numUnset++;
        else
        {
            totalWidth += w;
            if(totalWidth > width)
            {
                w = width - curWidth;
                totalWidth = width;
            }
            curWidth = totalWidth;
        }
    }

    const float numUnsetCols = static_cast<float>(_numCols - colWidths.size() + numUnset);
    if(numUnsetCols > 0.0f)
    {
        float w = 0;
        if(totalWidth < width)
        {
            const float remainingWidth = width - totalWidth;
            if(remainingWidth > numUnsetCols) w = remainingWidth / numUnsetCols;
        }
        for(size_t i = 0; i < colWidths.size(); ++i)
        {
            if(colWidths[i] == 0.0f) colWidths[i] = w;
        }
        for(size_t j = 0; j < _numCols - colWidths.size(); ++j)
        {
            colWidths.push_back(w);
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ComplexGridLayouter::calculateRowHeights(std::vector<float>& rowHeights, float height)
{
    float totalHeight = 0;
    float curHeight = 0;
    unsigned int numUnset = 0;
    for(SizeIterator it = rowHeights.begin(); it != rowHeights.end(); ++it)
    {
        float& w = *it;
        if(w == 0.0f)
            numUnset++;
        else
        {
            totalHeight += w;
            if(totalHeight > height)
            {
                w = height - curHeight;
                totalHeight = height;
            }
            curHeight = totalHeight;
        }
    }

    const float numUnsetRows = static_cast<float>(_numRows - rowHeights.size() + numUnset);
    if(numUnsetRows > 0.0f)
    {
        float h = 0;
        if(totalHeight < height)
        {
            const float remainingHeight = height - totalHeight;
            if(remainingHeight > numUnsetRows) h = remainingHeight / numUnsetRows;
        }
        for(int i = 0; i < int(rowHeights.size()); ++i)
        {
            if(rowHeights[i] == 0) rowHeights[i] = h;
        }
        for(unsigned j = 0; j < _numRows - rowHeights.size(); ++j)
        {
            rowHeights.push_back(h);
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ComplexGridLayouter::calculateBlocks()
{
    _numCols = 0;
    _numRows = 0;

    unsigned int nextCol = 0;
    unsigned int nextRow = 0;

    _blockMap.clear();

    std::list<Cell>::iterator it;
    for(it = _cells.begin(); it != _cells.end(); ++it)
    {
        const ComplexGridCellInfo& info = (*it).info;

        if ( info.columnIndex != ComplexGridCellInfo::relativeToPrevious )
            nextCol = info.columnIndex;

        if ( info.rowIndex != ComplexGridCellInfo::relativeToPrevious )
            nextRow = info.rowIndex;

        while( _blockMap.find(nextRow*256 + nextCol) != _blockMap.end() ) ++nextCol;

        unsigned int hSpan = info.horizontalSpan;
        unsigned int vSpan = info.verticalSpan;


        for( unsigned i = 0; i != hSpan; ++i )

        {
            for( unsigned j = 0; j != vSpan; ++j )
            {
                Block block;
                block.windowStart = ( i == 0 && j == 0 );
                block.col = nextCol + i;
                block.row = nextRow + j;
                block.hSpan = hSpan;
                block.vSpan = vSpan;
                block.hSpace = info.horizontalSpacing;
                block.vSpace = info.verticalSpacing;
                block.window = (*it).window;

                addBlockToMap(block);
            }
        }

        if ( nextCol + hSpan > _numCols )  _numCols = nextCol + hSpan;
        if ( nextRow + vSpan > _numRows )  _numRows = nextRow + vSpan;

        nextCol += hSpan;
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// **************************************************************************************************
//                            ComplexGridCellInfo
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string ComplexGridCellInfo::type_ = "ComplexGridCellInfo";
const unsigned int ComplexGridCellInfo::relativeToPrevious = 100000;
                                  //  std::numeric_limits<unsigned int>::max(); (MSVC can't handle this!!)
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComplexGridCellInfo::ComplexGridCellInfo() :
    horizontalSpan(1),
    verticalSpan(1),
    columnIndex(relativeToPrevious),
    rowIndex(relativeToPrevious),
    horizontalSpacing(3.0f),
    verticalSpacing(3.0f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// *************************************************************************************************
} // namespace Gooey
