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

#include "glgooey/MouseMessage.h"

#include "glgooey/core/Utils.h"
#include "glgooey/Window.h"
#include "glgooey/WindowManager.h"
#include "glgooey/core/Rectangle.h"





// **************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MouseMessage::MouseMessage(Id anId, const Core::Vector2& aPosition) :
    Message(anId),
    position_(aPosition)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MouseClickMessage::MouseClickMessage(Id anId, const Core::Vector2& aPosition,
                                 Button aButton, Event anEvent) :
    MouseMessage(anId, aPosition),
    button_(aButton),
    event_(anEvent)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// *************************************************************************************************
//                              MouseListener
// *************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MouseListener::MouseListener() :
    oldPosition_(-1e6f, -1e6f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<










// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
MouseListener::dispatchMessage(const Message& message, Window* receiverWindow)
{
    bool ret = false;

    Message::Id id = message.id();

    if( (id == Message::mouseClick)   ||
        (id == Message::mouseEntered) ||
        (id == Message::mouseExited)  ||
        (id == Message::mouseMoved) )
    {
        const MouseMessage& e = dynamic_cast<const MouseMessage&>(message);
        const Core::Vector2& newPosition = e.position();

        const bool newPosInWindow = receiverWindow->windowRectangle().surrounds(newPosition);
        const bool oldPosInWindow = receiverWindow->windowRectangle().surrounds(oldPosition_);
        //bool newPosInWindow = (receiverWindow == WindowManager::instance().windowBelowCursor());
        //bool oldPosInWindow = (receiverWindow == WindowManager::instance().oldWindowBelowCursor());

        const Window * capture = WindowManager::instance().captureWindow();

        // check for a mouse enter or exit on the receiver window
        if(id == Message::mouseMoved)
        {
            if((!capture && newPosInWindow) || capture == receiverWindow)
                ret = moved((MouseMessage&)(message));

            if( oldPosInWindow && !newPosInWindow )
            {
                MouseMessage rval(Message::mouseExited, newPosition);
                receiverWindow->process(rval);
            }
            else if( !oldPosInWindow && newPosInWindow )
            {
                MouseMessage rval(Message::mouseEntered, newPosition);
                receiverWindow->process(rval);
            }
            oldPosition_ = newPosition;
        }

        // pass the message on if the mouse is in the window
        switch(id)
        {
        case Message::mouseClick:
            if((!capture && newPosInWindow) || capture == receiverWindow)
                ret = ret || clicked((MouseClickMessage&)(message));
            break;

        case Message::mouseEntered:
            if((!capture && newPosInWindow) || capture == receiverWindow)
                ret = ret || entered((MouseMessage&)(message));
            break;

        case Message::mouseExited:
            if(!capture || capture == receiverWindow)
                ret = ret || exited((MouseMessage&)(message));
            break;

        default:
            break;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// **************************************************************************************************
} // namespace Gooey


