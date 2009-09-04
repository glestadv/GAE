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

#include "glgooey/ComboBox.h"

#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"
#include "glgooey/core/Rectangle.h"

#include "glgooey/MouseMessage.h"
#include "glgooey/WindowManager.h"
#include "glgooey/ListBox.h"
#include "glgooey/Renderer.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/SolidSurfaceDesc.h"


// *************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(ComboBoxDesc);


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Listener enabling single selection via the mouse
class ComboBoxMouseListener : public MouseListener, public SlotHolder
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor requires the list box the listener applies to
    ComboBoxMouseListener(ComboBox& aComboBox) : comboBox_(aComboBox) {}


    // ---------------------------------------------------------------------------------------------
    // Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to mouse clicks
    virtual bool clicked(MouseClickMessage& message)
    {
        bool isHandled = false;
        if(message.button() == MouseClickMessage::left)
        {
            isHandled = true;
            if(message.event() == MouseClickMessage::pressed)
            {
                if(comboBox_.listBox_ != 0)
                {
                    if(!comboBox_.isDropDownOpen_)
                    {
                        comboBox_.openDropDown();
                    }
                    else
                    {
                        comboBox_.closeDropDown();
                    }
                }
            }
        }
        return isHandled;
    }

    //! reacts to the selection in the list box changing
    ComboBoxMouseListener& listBoxSelectionChanged(int newVal)
    {
        comboBox_.selectStringAt(newVal);
        return *this;
    }

    //! reacts to the selection in the list box changing
    ComboBoxMouseListener& listBoxClicked()
    {
        comboBox_.closeDropDown();
        return *this;
    }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    ComboBox& comboBox_;

    // Copying and assignment are not allowed
    ComboBoxMouseListener(const ComboBoxMouseListener& aComboBoxMouseListener);
    ComboBoxMouseListener& operator = (const ComboBoxMouseListener& aComboBoxMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
//                                            ComboBox
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox::ComboBox(Window* aParent, const Core::Rectangle& rect) :
    Window(aParent),
    listBox_(0),
    renderDesc_(0),
    isDropDownOpen_(false)
{
    setPosition(rect.bottomLeft());
    setSize(rect.size());

    comboBoxMouseListener_ = new ComboBoxMouseListener(*this);
    addMessageListener(comboBoxMouseListener_);

    listBox_ = new ListBox(this, Core::Rectangle(rect.minX(), rect.maxY(), rect.maxX(), rect.maxY()+30));
    WindowManager::instance().addWindow(listBox_);
    listBox_->selectionChanged.connect(comboBoxMouseListener_,
                    &ComboBoxMouseListener::listBoxSelectionChanged);
    listBox_->leftClicked.connect(comboBoxMouseListener_,
                    &ComboBoxMouseListener::listBoxClicked);
    listBox_->hide();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox::~ComboBox()
{
    closeDropDown();

    WindowManager::instance().removeWindow(listBox_);
    delete listBox_;

    removeMessageListener(comboBoxMouseListener_);
    delete comboBoxMouseListener_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::addString(const std::string& aString)
{
    listBox_->addString(aString);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
ComboBox::stringAt(unsigned int position) const
{
    return listBox_->stringAt(position);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
ComboBox::selectedString() const
{
    return text();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int
ComboBox::selectedIndex() const
{
    return listBox_->selectedIndex();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::removeStringAt(unsigned int position)
{
    listBox_->removeStringAt(position);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::removeAllStrings()
{
    listBox_->removeAllStrings();
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned int
ComboBox::numberOfStrings() const
{
    return listBox_->numberOfStrings();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::selectString(const std::string& aString)
{
    if(!listBox_->contains(aString))
    {
        const unsigned int index = listBox_->numberOfStrings();
        (*listBox_)
            .addString(aString)
            .select(index)
            ;
    }
    else
    {
        const unsigned int index = listBox_->indexOf(aString);
        listBox_->select(index);
    }

    setText(aString);

    selectionChanged(aString);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::selectStringAt(unsigned int position)
{
    if( (position < numberOfStrings()) && (numberOfStrings() > 0) )
        selectString(listBox_->stringAt(position));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ComboBox::process(const Message& message)
{
    return Window::process(message);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
ComboBox::recommendedSize() const
{
    ComboBoxDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().comboBoxDesc_;

    renderDesc->textDesc().setText(text());
    return renderDesc->textDesc().textSize() + Core::Vector2(30, 8);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ComboBox::draw()
{
    ComboBoxDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().comboBoxDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->setComboBox(this);
    renderDesc->draw(rect);

    if(isDropDownOpen())
    {
        listBox_->setPosition(Core::Vector2(0, size().y()) + position());
        listBox_->setSize(Core::Vector2(size().x(), 110));
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ComboBox::onLostFocus()
{
    if(isDropDownOpen_)
    {
        bool hideListBox = true;
        if(WindowManager::instance().windowBelowCursor() != 0)
        {
            if(listBox_->doesHierarchyContain(WindowManager::instance().windowBelowCursor()))
            {
                hideListBox = false;
            }
        }
        if(hideListBox)
        {
            listBox_->hide();
            isDropDownOpen_ = false;
            listClosed();
        }
    }
    Window::onLostFocus();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::openDropDown()
{
    listBox_->show();
    isDropDownOpen_ = true;
    listOpened();
    WindowManager::instance().setPopUpWindow(listBox_);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBox&
ComboBox::closeDropDown()
{
    listBox_->hide();
    isDropDownOpen_ = false;
    listClosed();
    WindowManager::instance().setPopUpWindow(0);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ComboBox::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ComboBoxDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<ComboBoxDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                            ComboBoxDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBoxDesc::ComboBoxDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()),
    textDesc_(new TextDesc)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ComboBoxDesc::~ComboBoxDesc()
{
    delete background_;
    delete border_;
    delete textDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ComboBoxDesc::draw(const Core::Rectangle& area) const
{
    using namespace Core;

    textDesc_->setHorizontalAlignment(Alignment::left);

    if(background_ != 0) background_->draw(area);
    if(border_ != 0)     border_->draw(area);

    const Vector2 topLeft = comboBox_->position();
    const Vector2 bottomRight = comboBox_->position() + comboBox_->size();
    Core::Rectangle rect(topLeft, bottomRight);

    // draw the text
    textDesc_->setText(comboBox_->text());
    Vector2 textSize = textDesc_->textSize();

    Vector2 halfSize = (bottomRight - topLeft) * 0.5f;
    Vector2 halfTextSize = textSize * 0.5f;

    Vector2 offset(6.0f, halfSize.y() - halfTextSize.y());

    Core::Rectangle textRect = area;
    textRect.setSize(Vector2(area.size().x() - 20.0f, area.size().y()));
    Renderer::enableClipRectangle(textRect);
    textDesc_->draw(textRect);
    Renderer::disableClipRectangle();

    Renderer::setCurrentColor(textDesc_->color());

    // get the points that make up the arrow
    Vector2 points[3];
    const Vector2 center = bottomRight + Vector2(-10.0f, -halfSize.y());
    float horzOffset = halfSize.y() / 3.0f;
    if(horzOffset < 4.0f) horzOffset = 4.0f;
    if(horzOffset > 8.0f) horzOffset = 8.0f;
    float vertOffset = halfSize.y() / 3.0f;
    if(vertOffset < 4.0f) vertOffset = 4.0f;
    if(vertOffset > 8.0f) vertOffset = 8.0f;
    points[0] = center + Vector2(-horzOffset, -vertOffset);
    points[1] = center + Vector2( 0.0f,        vertOffset);
    points[2] = center + Vector2( horzOffset, -vertOffset);

    // draw the arrow
    Renderer::drawTriangle(points[0], points[1], points[2], true);

    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Gooey
