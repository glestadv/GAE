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

#include "glgooey/KeyMessage.h"

#include "glgooey/Window.h"
#include "glgooey/core/Utils.h"



// *************************************************************************************************
namespace Gooey
{

const unsigned short  KeyMessage::GOOEY_F1           = 1;
const unsigned short  KeyMessage::GOOEY_F2           = 2;
const unsigned short  KeyMessage::GOOEY_F3           = 3;
const unsigned short  KeyMessage::GOOEY_F4           = 4;
const unsigned short  KeyMessage::GOOEY_F5           = 5;
const unsigned short  KeyMessage::GOOEY_F6           = 6;
const unsigned short  KeyMessage::GOOEY_F7           = 7;
const unsigned short  KeyMessage::GOOEY_F8           = 8;
const unsigned short  KeyMessage::GOOEY_F9           = 9;
const unsigned short  KeyMessage::GOOEY_F10          = 10;
const unsigned short  KeyMessage::GOOEY_F11          = 11;
const unsigned short  KeyMessage::GOOEY_F12          = 12;
const unsigned short  KeyMessage::GOOEY_LEFT         = 100;
const unsigned short  KeyMessage::GOOEY_UP           = 101;
const unsigned short  KeyMessage::GOOEY_RIGHT        = 102;
const unsigned short  KeyMessage::GOOEY_DOWN         = 103;
const unsigned short  KeyMessage::GOOEY_PAGE_UP      = 104;
const unsigned short  KeyMessage::GOOEY_PAGE_DOWN    = 105;
const unsigned short  KeyMessage::GOOEY_HOME         = 106;
const unsigned short  KeyMessage::GOOEY_END          = 107;
const unsigned short  KeyMessage::GOOEY_INSERT       = 108;



// **************************************************************************************************
//                         KeyMessage
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
KeyMessage::KeyMessage(Id anId, unsigned char aCharacter, unsigned short aKeyCode) :
    Message(anId),
    character_(aCharacter),
    keyCode_(aKeyCode)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
//                         KeyListener
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
KeyListener::dispatchMessage(const Message& message, Window* receiverWindow)
{
    bool ret = false;
    if(receiverWindow->isFocused())
    {
        switch(message.id())
        {
        case Message::characterInput:
            onCharacterInput((dynamic_cast<const KeyMessage&>(message)).character());
            break;
        case Message::keyPressed:
            onKeyPressed((dynamic_cast<const KeyMessage&>(message)).keyCode());
            break;
        case Message::keyReleased:
            onKeyReleased((dynamic_cast<const KeyMessage&>(message)).keyCode());
            break;
        default:
            break; // Default case is don't do anything
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey


