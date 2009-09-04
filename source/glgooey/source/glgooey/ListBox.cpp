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

#include "glgooey/ListBox.h"

#include "glgooey/SelectionMessage.h"
#include "glgooey/Renderer.h"
#include "glgooey/WindowManager.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/ScrollBar.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/SolidSurfaceDesc.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"



// **************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(ListBoxDesc);



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListBox::ListBox(Window* aParent, const Core::Rectangle& rect) :
    ListWindow(aParent, rect),
    renderDesc_(0)
{
    setSize(rect.size());
    setPosition(rect.bottomLeft());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListBox::~ListBox()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListBox&
ListBox::addString(const std::string& aString)
{
    strings_.push_back(aString);

    onContentsChanged();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListBox&
ListBox::removeStringAt(unsigned int pos)
{
    if(selectedIndex() == static_cast<signed int>(pos)) select(-1);
    if(pos < numberOfStrings())
    {
        Strings::iterator it = strings_.begin();
        std::advance(it, pos);
        strings_.erase(it);
    }

    onContentsChanged();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListBox&
ListBox::removeAllStrings()
{
    select(-1);

    strings_.clear();

    onContentsChanged();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
ListBox::stringAt(unsigned int position) const
{
    std::string ret;
    if(position < numberOfStrings())
    {
        Strings::const_iterator it = strings_.begin();
        std::advance(it, position);
        ret = *it;
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListBox::contains(const std::string& aString) const
{
    return ( std::find(strings_.begin(), strings_.end(), aString) != strings_.end() );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned int
ListBox::indexOf(const std::string& aString) const
{
    int ret = -1;
    if(numberOfStrings() > 0)
    {
        ret = 0;
        for(Strings::const_iterator it = strings_.begin(); it != strings_.end(); ++it)
        {
            if((*it) == aString) break;
            ++ret;
        }
    }
    if((ret < 0) || (ret >= int(numberOfStrings())))
        THROW("Tried to get the index of an invalid string");

    return static_cast<unsigned int>(ret);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ListBox::draw()
{
    ListBoxDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().listBoxDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->setListWindow(this);
    renderDesc->draw(rect);

    _verticalScrollBar->render();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ListBox::entryHeight() const
{
    ListBoxDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().listBoxDesc_;
    return renderDesc->textDesc().lineHeight();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ListBox::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ListBoxDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<ListBoxDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void
ListBoxDesc::drawRow(size_t aRowIndex, const Core::Rectangle& aRowRectangle) const
{
    ListBox* listBox = dynamic_cast<ListBox*>(listWindow_);
    
    textDesc_->setText(listBox->stringAt(aRowIndex));

    Core::Rectangle textRectangle = aRowRectangle;
    textRectangle.translateBy(Core::Vector2(2.0f, 0.0f));

    if(static_cast<int>(aRowIndex) == listBox->selectedIndex())
    {
        Renderer::setCurrentColor(selectionColor_);
        Renderer::drawRectangle(aRowRectangle.topLeft(), aRowRectangle.bottomRight(), true);

        const Color unselectedColor = textDesc_->color();
        (*textDesc_)
            .setColor(selectedTextColor_)
            .draw(textRectangle)
            ;
        textDesc_->setColor(unselectedColor);
    }
    else if(static_cast<int>(aRowIndex) == listBox->highlightedIndex())
    {
        Renderer::setCurrentColor(highlightColor_);
        Renderer::drawRectangle(aRowRectangle.topLeft(), aRowRectangle.bottomRight(), true);

        const Color unselectedColor = textDesc_->color();
        (*textDesc_)
            .setColor(highlightedTextColor_)
            .draw(textRectangle)
            ;
        textDesc_->setColor(unselectedColor);
    }
    else
    {
        textDesc_->draw(textRectangle);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// **************************************************************************************************
} // namespace Gooey

