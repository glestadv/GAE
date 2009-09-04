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

#include "glgooey/BasicButton.h"

#include "glgooey/Renderer.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/WindowManager.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/SolidSurfaceDesc.h"
#include "glgooey/BasicButtonGroup.h"
#include "glgooey/SelectionMessage.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"





// *************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(ButtonStateDesc);
REGISTER_CLASS(ButtonDesc);


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Mouse listener for the button class
//! \see Button
class ButtonMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor requires the button it is to be applied to
    ButtonMouseListener(BasicButton& aButton) : button_(aButton) {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to mouse clicks
    virtual bool clicked(MouseClickMessage& message)
    {
        bool isHandled = false;
        if(message.button() == MouseClickMessage::left)
        {
            isHandled = true;
            if( (message.event() == MouseClickMessage::pressed) ||
                (message.event() == MouseClickMessage::doubleClicked) )
            {
                if(WindowManager::instance().captureWindow() != &button_)
                    WindowManager::instance().setMouseCapture(&button_);
                button_.captureCount_++;
                button_.state_ = BasicButton::down;
                button_.pressed();

            }
            else if(message.event() == MouseClickMessage::released)
            {
                if(&button_ == button_.windowBelow(message.position()))
                {
                    button_.state_ = BasicButton::rollOver;
                    button_.released();
                    button_.clicked();
                }
                else
                {
                    button_.state_ = BasicButton::up;
                }

                if(button_.captureCount_ == 1)
                    WindowManager::instance().releaseMouseCapture();
                if(button_.captureCount_ > 0)
                    button_.captureCount_--;
            }
        }
        return isHandled;
    }

    // react to the mouse leaving the button
    virtual bool exited(MouseMessage& GOOEY_UNUSED(message))
    {
        if(button_.state_ != BasicButton::down)
            button_.state_ = BasicButton::up;
        else
        {
            if(!button_.keepPressedOnExit_)
            {
                button_.released();
                button_.state_ = BasicButton::up;
            }
        }
        return true;
    }

    // react to the mouse entering the button
    virtual bool entered(MouseMessage& GOOEY_UNUSED(message))
    {
        if(button_.state_ != BasicButton::down)
            button_.state_ = BasicButton::rollOver;
        if(button_.captureCount_ > 0)
        {
            button_.state_ = BasicButton::down;
            button_.pressed();
        }
        return true;
    }


    // react to the mouse moved message
    virtual bool moved(MouseMessage& GOOEY_UNUSED(message))
    {
        return button_.state_ == BasicButton::rollOver;
    }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    BasicButton& button_;

    // Copying and assignment are not allowed
    ButtonMouseListener(const ButtonMouseListener& aButtonMouseListener);
    ButtonMouseListener& operator = (const ButtonMouseListener& aButtonMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
//                                            BasicButton
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton::BasicButton(Window* aParent, const std::string& aText,BasicButtonGroup* aGroup) :
    Window(aParent, aText),
    state_(up),
    captureCount_(0),
    keepPressedOnExit_(false),
    isToggleOn_(false),
    isMouseOver_(false),
    group_(aGroup),
    renderDesc_(0)
{
    // instantiate the listener
    _buttonMouseListener = new ButtonMouseListener(*this);
    addMessageListener(_buttonMouseListener);

    if(group_ != 0)
        group_->buttons_.push_back(this);

    pressed.connect(this, &BasicButton::onPressedLeft);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton::~BasicButton()
{
    removeMessageListener(_buttonMouseListener);
    delete _buttonMouseListener;

    if(group_ != 0)
        group_->buttons_.remove(this);

    delete renderDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton&
BasicButton::setMouseOverStatus(bool isMouseOverButton)
{
    if(isToggleOn_)
    {
        if (isMouseOverButton && !isMouseOver_)
        {
            isMouseOver_ = true;

            if(group_ != 0)
            {
                process(SelectionMessage(Message::selected, group_->indexOf(this)));
                BasicButtonGroup::BasicButtons::iterator it;
                for(it = group_->buttons_.begin(); it != group_->buttons_.end(); ++it)
                {
                    BasicButton* btn = *it;
                    if(btn != this) btn->setMouseOverStatus(false);
                }
            }
            else
            {
                process(SelectionMessage(Message::selected, 0));
                toggled(isMouseOver());
            }
        }
        else if (!isMouseOverButton && isMouseOver_)
        {
            bool canDeselect = true;
            if(group_ != 0)
            {
                //  if this checkbox is in a group, we have to make sure that
                // another box is selected bfore we deselect this one
                canDeselect = false;
                BasicButtonGroup::BasicButtons::iterator it;
                for(it = group_->buttons_.begin(); it != group_->buttons_.end(); ++it)
                {
                    BasicButton* btn = *it;
                    if(btn != this)
                    {
                        if(btn->isMouseOver()) canDeselect = true;
                    }
                }
            }
            if(canDeselect)
            {
                isMouseOver_ = false;
                if(group_ != 0) {
                    process(SelectionMessage(Message::deselected, group_->indexOf(this)));
                    toggled(isMouseOver());
                }
                else {
                    process(SelectionMessage(Message::deselected, 0));
                    toggled(isMouseOver());
                }
            }
        }

    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton::State
BasicButton::drawState() const
{
    State s = state();

    if (!isToggleOn_)
        return s;

    return s==rollOver ? (isMouseOver()?rollOverOn:rollOver) : (isMouseOver()?down:up);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton&
BasicButton::toggle()
{
    if (isToggleOn_)
        setMouseOverStatus(!isMouseOver());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton&
BasicButton::onPressedLeft()
{
    if(isToggleOn_)
        setMouseOverStatus(!isMouseOver_);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton&
BasicButton::keepPressedOnExit()
{
    keepPressedOnExit_ = true;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
BasicButton&
BasicButton::drawStandardButton()
{
    Core::Rectangle rect(position(), position() + size());

    buttonDesc().draw(rect);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
BasicButton::recommendedSize() const
{
    return buttonDesc().defaultTextSize() + Core::Vector2(8, 8);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonDesc&
BasicButton::buttonDesc() const
{
    ButtonDesc* result = renderDesc_;
    if(result == 0)
        result = &WindowManager::instance().propertyScheme().buttonDesc_;

    result->setButton(*this);

    return *result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
BasicButton::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ButtonDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::nameObject<ButtonDesc>(*renderDesc_, aWindowName.c_str());
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                            ButtonStateDesc
// **************************************************************************************************






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonStateDesc::ButtonStateDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()),
    textDesc_(new TextDesc()),
    button_(0)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonStateDesc::ButtonStateDesc(RenderDesc* aBackground, RenderDesc* aBorder) :
    background_(aBackground),
    border_(aBorder),
    textDesc_(new TextDesc()),
    button_(0)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonStateDesc::~ButtonStateDesc()
{
    delete background_;
    delete border_;
    delete textDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Color
ButtonStateDesc::textColor() const
{
    return (textDesc_ != 0) ? textDesc_->color() : Color();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ButtonStateDesc::draw(const Core::Rectangle& anArea) const
{
    if(background_ != 0) background_->draw(anArea);
    if(border_ != 0)     border_->draw(anArea);

    if(button_->text() != "")
    {
        Core::Rectangle area = anArea;

        const Core::Vector2 topLeft = button_->position();
        const Core::Vector2 bottomRight = topLeft + button_->size();

        // draw the text
        textDesc_->setText(button_->text());

        const bool isCheckBox = dynamic_cast<const CheckBox*>(button_) != 0;

        if(isCheckBox)
        {
            area.translateBy(Core::Vector2(anArea.size().y(), 0));
            area.setSize(Core::Vector2(anArea.size().x() - anArea.size().y(), anArea.size().y()));
        }
        textDesc_->draw(area);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// **************************************************************************************************
//                                            ButtonDesc
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonDesc::ButtonDesc() :
    pressedDesc_(new SolidSurfaceDesc(Color(0.2f, 0.2f, 0.4f)), new RectangleDesc(dropped)),
    releasedDesc_(new SolidSurfaceDesc(), new RectangleDesc(raised)),
    rollOverDesc_(new SolidSurfaceDesc(Color(0.35f, 0.35f, 0.5f, 1)), new RectangleDesc(raised)),
    rollOverOnDesc_(new SolidSurfaceDesc(Color(0.25f, 0.25f, 0.45f)), new RectangleDesc(dropped)),
    arrow_(none),
    button_(0)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ButtonDesc::~ButtonDesc()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
ButtonDesc::defaultTextSize() const
{
    ASSERT(button_ != 0);

    TextDesc& textDesc = releasedDesc_.textDesc();
    textDesc.setText(button_->text());
    return textDesc.textSize() + Core::Vector2(8.0f, 8.0f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ButtonDesc::draw(const Core::Rectangle& GOOEY_UNUSED(area)) const
{
    const Core::Vector2 topLeft = button_->position();
    const Core::Vector2 bottomRight = topLeft + button_->size();

    Core::Rectangle rect(topLeft, bottomRight);
    Renderer::enableClipRectangle(rect);

    Color textColor;
    switch(button_->drawState())
    {
    case BasicButton::rollOverOn:
        rollOverOnDesc_.setButton(*button_);
        rollOverOnDesc_.draw(rect);
        textColor = rollOverOnDesc_.textColor();
        break;
    case BasicButton::down:
        pressedDesc_.setButton(*button_);
        pressedDesc_.draw(rect);
        textColor = pressedDesc_.textColor();
        break;
    case BasicButton::rollOver:
        rollOverDesc_.setButton(*button_);
        rollOverDesc_.draw(rect);
        textColor = rollOverDesc_.textColor();
        break;
    case BasicButton::up:
        releasedDesc_.setButton(*button_);
        releasedDesc_.draw(rect);
        textColor = releasedDesc_.textColor();
        break;
    }

    if(arrow_ != none)
    {
        // get the points that make up the arrow
        Core::Vector2 points[3];
        const Core::Vector2 center = rect.centerPoint();
        const float horzOffset = rect.size().x() * 0.25f;
        const float vertOffset = rect.size().y() * 0.25f;
        switch(arrow_)
        {
        case ButtonDesc::up:
            points[0] = center + Core::Vector2(-horzOffset,  vertOffset);
            points[1] = center + Core::Vector2( 0.0f,       -vertOffset);
            points[2] = center + Core::Vector2( horzOffset,  vertOffset);
            break;
        case ButtonDesc::down:
            points[0] = center + Core::Vector2(-horzOffset, -vertOffset);
            points[1] = center + Core::Vector2( 0.0f,        vertOffset);
            points[2] = center + Core::Vector2( horzOffset, -vertOffset);
            break;
        case ButtonDesc::left:
            points[0] = center + Core::Vector2( horzOffset, -vertOffset);
            points[1] = center + Core::Vector2(-horzOffset,  0.0f      );
            points[2] = center + Core::Vector2( horzOffset,  vertOffset);
            break;
        case ButtonDesc::right:
            points[0] = center + Core::Vector2(-horzOffset, -vertOffset);
            points[1] = center + Core::Vector2( horzOffset,  0.0f      );
            points[2] = center + Core::Vector2(-horzOffset,  vertOffset);
            break;
        case ButtonDesc::none:
            THROW("Invalid arrow direction");
        }
        arrow_ = none;
        Renderer::setCurrentColor(textColor);
        Renderer::drawTriangle(points[0], points[1], points[2], true);
    }
    Renderer::setCurrentColor(Color(1, 1, 1, 1));
    Renderer::disableClipRectangle();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey
