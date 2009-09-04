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

#include "glgooey/MultiTextButton.h"

#include "glgooey/MouseMessage.h"
#include "glgooey/WindowManager.h"




// **************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Processs clicks for the MultiTextButton control
class MTButtonMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires the button, that this listener is associated with
    MTButtonMouseListener(MultiTextButton& aMTButton) : button_(aMTButton) {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in base class
    virtual bool clicked(MouseClickMessage& message)
    {
        bool isHandled = false;
        if(message.button() == MouseClickMessage::right)
        {
            isHandled = true;
            if(message.event() == MouseClickMessage::pressed)
            {
                if(WindowManager::instance().captureWindow() != &button_)
                    WindowManager::instance().setMouseCapture(&button_);
                button_.captureCount_++;
                button_.state_ = BasicButton::down;
            }
            else if(message.event() == MouseClickMessage::released)
            {
                if(&button_ == button_.windowBelow(message.position()))
                {
                    button_.state_ = BasicButton::rollOver;
                    button_.onPressedRight();
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


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    MultiTextButton& button_;

    // Copying and assignment are not allowed
    MTButtonMouseListener(const MTButtonMouseListener& anMTButtonMouseListener);
    MTButtonMouseListener& operator = (const MTButtonMouseListener& anMTButtonMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                      MultiTextButton
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MultiTextButton::MultiTextButton(Window* aParent, const std::string& aText) :
    BasicButton(aParent, aText)
{
    addString(aText);
    stringIndex_ = 0;

    // instantiate the listeners
    mtButtonMouseListener_ = new MTButtonMouseListener(*this);
    addMessageListener(mtButtonMouseListener_);

    released.connect(this, &MultiTextButton::onPressedLeft);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MultiTextButton::~MultiTextButton()
{
    removeMessageListener(mtButtonMouseListener_);
    delete mtButtonMouseListener_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MultiTextButton&
MultiTextButton::onPressedLeft()
{
    if(numberOfStrings() == 0)
        setText("");
    else
    {
        stringIndex_++;
        if(stringIndex_ >= numberOfStrings()) stringIndex_ = 0;

        setText(stringAt(stringIndex_));

        selectionChanged(text());
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MultiTextButton&
MultiTextButton::onPressedRight()
{
    if(numberOfStrings() == 0)
        setText("");
    else
    {
        if(stringIndex_ == 0) stringIndex_ = numberOfStrings();
        stringIndex_--;

        setText(stringAt(stringIndex_));

        selectionChanged(text());
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
MultiTextButton::draw()
{
    drawStandardButton();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
} // namespace Gooey

