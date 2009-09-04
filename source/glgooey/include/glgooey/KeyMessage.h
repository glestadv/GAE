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
#ifndef KEYMESSAGE__H
#define KEYMESSAGE__H

#include "mmgr/mmgr.h"

#include "Message.h"
#include "glgooey/MessageListener.h"


// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Message caused by interevent with the keyboard
class KeyMessage : public Message
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor
    KeyMessage(Id anId, unsigned char character, unsigned short aKeyCode);

    //! destructor
    virtual ~KeyMessage() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief returns the character corresponding to the key pressed in a characterInput message
    unsigned char character() const { return character_; }

    //! returns the code of the key pressed in a keyPressed message
    unsigned short keyCode() const { return keyCode_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
public:
    static const unsigned short  GOOEY_F1         ;
    static const unsigned short  GOOEY_F2         ;
    static const unsigned short  GOOEY_F3         ;
    static const unsigned short  GOOEY_F4         ;
    static const unsigned short  GOOEY_F5         ;

    static const unsigned short  GOOEY_F6         ;
    static const unsigned short  GOOEY_F7         ;
    static const unsigned short  GOOEY_F8         ;
    static const unsigned short  GOOEY_F9         ;
    static const unsigned short  GOOEY_F10        ;
    static const unsigned short  GOOEY_F11        ;
    static const unsigned short  GOOEY_F12        ;
    static const unsigned short  GOOEY_LEFT       ;
    static const unsigned short  GOOEY_UP         ;
    static const unsigned short  GOOEY_RIGHT      ;
    static const unsigned short  GOOEY_DOWN       ;
    static const unsigned short  GOOEY_PAGE_UP    ;
    static const unsigned short  GOOEY_PAGE_DOWN  ;
    static const unsigned short  GOOEY_HOME       ;
    static const unsigned short  GOOEY_END        ;
    static const unsigned short  GOOEY_INSERT     ;

private:
    // the character corresponding to the key pressed in a characterInput message
    unsigned char character_;

    // the code of the key pressed in a keyPressed message
    unsigned short keyCode_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Listener for keyboard messages
class KeyListener : public MessageListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! destructor
    virtual ~KeyListener() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to the user entering a character
    virtual bool onCharacterInput(unsigned char ) { return false; }

    //! react to the user pressing a key
    virtual bool onKeyPressed(unsigned short ) { return false; }

    //! react to the user releasing a key
    virtual bool onKeyReleased(unsigned short ) { return false; }

    //! \brief see if the message can be interpreted by this class and if so pass the
    //! message on to the recipient method
    virtual bool dispatchMessage(const Message& message, Window* receiverWindow);

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
