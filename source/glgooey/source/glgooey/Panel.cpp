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

#include "glgooey/Panel.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/FlowLayouter.h"
#include "glgooey/WindowManager.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"


// *************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(PanelDesc);



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Panel::Panel(const Core::Rectangle& rect, Window* aParent, const std::string& aText) :
    Window(aParent, aText),
    layouter_(new FlowLayouter),
    renderDesc_(0)
{
    // set the geometry
    setWindowRectangle(rect);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Panel::~Panel()
{
    delete layouter_;
    delete renderDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
Panel::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
    {
        for(std::list<Window*>::iterator it = windows_.begin();
            it != windows_.end(); ++it)
        {
            Window* window = *it;
            ret = window->process(message);
            if(ret) break;
        }
    }
    if(!ret) Window::process(message);

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
Panel::doesHierarchyContain(Window* aWindow) const
{
    bool ret = false;
    for(std::list<Window*>::const_iterator it = windows_.begin();
        it != windows_.end(); ++it)
    {
        Window* window = *it;
        if(window->doesHierarchyContain(aWindow))
        {
            ret = true;
            break;
        }
    }
    return (ret || Window::doesHierarchyContain(aWindow));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
Panel::recommendedSize() const
{
    return Core::Vector2(150, 150);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
Panel::draw()
{
    PanelDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().panelDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->draw(rect);

    // ***********************************************************************
    //  draw the children
    // ***********************************************************************

    std::list<Window*>::iterator it;
    for(it = windows_.begin(); it != windows_.end(); ++it)
    {
        (*it)->render();
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window*
Panel::windowBelow(const Core::Vector2& p)
{
    Window* ret = Window::windowBelow(p);

    if(ret != 0)
    {
        std::list<Window*>::iterator it;
        for(it = windows_.begin(); it != windows_.end(); ++it)
        {
            Window* window = *it;
            Window* cw = window->windowBelow(p);
            if(cw != 0)
            {
                ret = cw;
                break;
            }
        }
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Panel::onResize()
{
    arrangeChildren();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Panel::onMove()
{
    arrangeChildren();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Panel&
Panel::setLayouter(BaseLayouter* aLayouter)

{
    if(aLayouter != 0)
    {
        delete layouter_;
        layouter_ = aLayouter;
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
Panel::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new PanelDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<PanelDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
PanelDesc::draw(const Core::Rectangle& area) const
{
    if(background_ != 0) background_->draw(area);
    if(border_ != 0)     border_->draw(area);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Gooey


