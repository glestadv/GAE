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

#include <cmath>

#include "glgooey/GridLayouter.h"

#include "glgooey/Window.h"


// *************************************************************************************************
namespace Gooey
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GridLayouter::GridLayouter() :
    numberOfRows_(0),
    numberOfColumns_(1),
    horizontalSpacing_(4),
    verticalSpacing_(4)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GridLayouter::GridLayouter(size_t numRows, size_t numColumns) :
    numberOfRows_(numRows),
    numberOfColumns_(numColumns),
    horizontalSpacing_(4),
    verticalSpacing_(4)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GridLayouter::GridLayouter(size_t numRows, size_t numColumns, size_t aHorizontalSpacing, size_t aVerticalSpacing) :
    numberOfRows_(numRows),
    numberOfColumns_(numColumns),
    horizontalSpacing_(aHorizontalSpacing),
    verticalSpacing_(aVerticalSpacing)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
GridLayouter::addWindow(Window* window)
{
    windows_.push_back(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
GridLayouter::removeWindow(Window* window)
{
    windows_.remove(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
GridLayouter::arrangeWithin(Window* parentWindow)
{
    const float width = parentWindow->size().x();
    const float height = parentWindow->size().y();

    const size_t numWindows = windows_.size();

    if(numWindows > 0)
    {
        size_t nr = numberOfRows();
        size_t nc = numberOfColumns();
        if(nr == 0)
        {
            if(nc == 0) nc = 1;
            else if(nc > numWindows) nc = numWindows;
            nr = (size_t)(ceil(float(numWindows) / float(nc)));
        }
        else
        {
            if(nr > numWindows) nr = numWindows;
            if(numberOfColumns() == 0)
                nc = (size_t)(ceil(float(numWindows) / float(nr)));
        }

        const float numRows = static_cast<float>(nr);
        const float numCols = static_cast<float>(nc);

        float childWidth  = (width - horizontalSpacing() -
            (numCols * horizontalSpacing())) / numCols;
        float childHeight = (height - verticalSpacing() -
            (numRows * verticalSpacing())) / numRows;

        std::list<Window*>::iterator it = windows_.begin();

        float y = static_cast<float>(verticalSpacing());
        for(size_t j = 0; j < numRows; ++j)
        {
            float x = static_cast<float>(horizontalSpacing());
            float maxHeight = 0;
            for(size_t i = 0; i < numCols; ++i)
            {
                Window* window = *it;
                window->setPosition(Core::Vector2(x, y) + parentWindow->position());
                window->setSize(Core::Vector2(childWidth, childHeight));
                const float deltaWidth = childWidth - window->size().x();
                if(i < numCols-1) childWidth += deltaWidth / static_cast<float>(numCols-i-1);
                x += window->size().x() + horizontalSpacing();
                if(window->size().y() > maxHeight) maxHeight = window->size().y();
                ++it;
                if(it == windows_.end()) break;
            }
            y += maxHeight + verticalSpacing();
            float deltaHeight = childHeight - maxHeight;
            if(j < numRows-1) childHeight += deltaHeight / static_cast<float>(numRows-j-1);
        }
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
} // namespace Gooey

