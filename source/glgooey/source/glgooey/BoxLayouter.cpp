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

#include "glgooey/BoxLayouter.h"

#include "glgooey/Window.h"



// *************************************************************************************************
namespace Gooey
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BoxLayouter::BoxLayouter(Justification aJustification) :
    justification_(aJustification),
    spacing_(4.0f),
    isChildSizeRespected_(true)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BoxLayouter::BoxLayouter(Justification aJustification, float aSpacing, bool respectChildSizes) :
    justification_(aJustification),
    spacing_(aSpacing),
    isChildSizeRespected_(respectChildSizes)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
BoxLayouter::addWindow(Window* window)
{
    windows_.push_back(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
BoxLayouter::removeWindow(Window* window)
{
    windows_.remove(window);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BaseLayouter&
BoxLayouter::arrangeWithin(Window* parentWindow)
{
    const float width = parentWindow->size().x();
    const float height = parentWindow->size().y();

    if(!isChildSizeRespected())
    {
        float windowWidth  = 0.0f;
        float windowHeight = 0.0f;
        float numWindows = static_cast<float>(windows_.size());
        if(justification() == horizontal)
        {
            windowWidth  = (width - (spacing() * (numWindows + 1.0f))) / numWindows;
            windowHeight = (height - (spacing() * 2.0f));
        }
        else
        {
            windowWidth  = (width - (spacing() * 2.0f));
            windowHeight = (height - (spacing() * (numWindows + 1.0f))) / numWindows;
        }

        for(std::list<Window*>::iterator it = windows_.begin(); it != windows_.end(); ++it)
        {
            (*it)->setSize(Core::Vector2(windowWidth, windowHeight));
        }
    }


    if(justification() == horizontal)
    {
        const float parentCenter = parentWindow->position().y() + (height * 0.5f);
        float x = parentWindow->position().x() + spacing_;
        for(std::list<Window*>::iterator it = windows_.begin(); it != windows_.end(); ++it)
        {
            Window* window = *it;
            const float y = parentCenter - (window->size().y() * 0.5f);
            window->setPosition(Core::Vector2(x, y));
            x += window->size().x() + spacing_;
        }
    }
    else
    {
        const float parentCenter = parentWindow->position().x() + (width * 0.5f);
        float y = parentWindow->position().y() + spacing_;
        for(std::list<Window*>::iterator it = windows_.begin(); it != windows_.end(); ++it)
        {
            Window* window = *it;
            const float x = parentCenter - (window->size().x() * 0.5f);
            window->setPosition(Core::Vector2(x, y));
            y += window->size().y() + spacing_;
        }
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Gooey
