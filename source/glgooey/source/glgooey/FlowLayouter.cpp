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

#include "glgooey/FlowLayouter.h"

#include "glgooey/Window.h"



// *************************************************************************************************
namespace Gooey
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FlowLayouter::FlowLayouter(Alignment anAlignment) :
    alignment_(anAlignment),
    horizontalSpacing_(4.0f),
    verticalSpacing_(4.0f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FlowLayouter::FlowLayouter(Alignment anAlignment,
                           float aHorizontalSpacing, float aVerticalSpacing) :
    alignment_(anAlignment),
    horizontalSpacing_(aHorizontalSpacing),
    verticalSpacing_(aVerticalSpacing)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
FlowLayouter::addWindow(Window* window)
{
    windows_.push_back(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
FlowLayouter::removeWindow(Window* window)
{
    windows_.remove(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
FlowLayouter::arrangeWithin(Window* parentWindow)
{
    const float width = parentWindow->size().x();
    const float height = parentWindow->size().y();

    std::list<Window*>::iterator firstInRow = windows_.begin();
    std::list<Window*>::iterator lastInRow = firstInRow;

    bool done = false;

    if(width > horizontalSpacing())
    {
        float y = verticalSpacing();
        while(!done)
        {
            float x = horizontalSpacing();
            bool isRowDone = false;
            while(!isRowDone)
            {
                if(lastInRow == windows_.end())
                {
                    done = true;
                    isRowDone = true;
                }
                else
                {
                    Window* window = (*lastInRow);
                    float windowWidth = window->size().x();
                    if(windowWidth + 2.0f*horizontalSpacing() > width)
                    {
                        windowWidth = width - 2.0f*horizontalSpacing();
                        window->setSize(Core::Vector2(windowWidth, window->size().y()));
                    }
                    float windowHeight = window->size().y();
                    if(windowHeight + 2.0f*verticalSpacing() > height)
                    {
                        windowHeight = width - 2.0f*verticalSpacing();
                        window->setSize(Core::Vector2(window->size().x(), windowHeight));
                    }

                    if(x + windowWidth + horizontalSpacing() <= width)
                    {
                        x += windowWidth + horizontalSpacing();
                        ++lastInRow;
                    }
                    else
                    {
                        isRowDone = true;
                    }
                }
            }


            const float totalWidth = x;
            switch(alignment_)
            {
            case left:
                x = horizontalSpacing();
                break;
            case right:
                x = width - totalWidth + horizontalSpacing();
                break;
            case center:
                x = (width * 0.5f) - (totalWidth * 0.5f) + horizontalSpacing();
                break;
            }

            float maxY = 0.0f;
            for(std::list<Window*>::iterator it = firstInRow; it != lastInRow; ++it)
            {
                Window* window = (*it);
                window->setPosition(Core::Vector2(x, y) + parentWindow->position());
                float windowWidth = window->size().x();
                x += windowWidth + horizontalSpacing();
                if(window->size().y() > maxY) maxY = window->size().y();
            }
            y += maxY + verticalSpacing();
            firstInRow = lastInRow;
        }
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Gooey
