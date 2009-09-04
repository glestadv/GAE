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
#ifndef MOUSEMESSAGE__H
#define MOUSEMESSAGE__H

#include "mmgr/mmgr.h"

#include "Message.h"
#include "glgooey/MessageListener.h"
#include "glgooey/core/Vector2.h"




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Message caused by interaction with the mouse
class MouseMessage : public Message
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires the id and the mouse position
    MouseMessage(Id anId, const Core::Vector2& aPosition);

    //! destructor
    virtual ~MouseMessage() {}



    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the position of the mouse cursor when the message occured
    Core::Vector2 position() const { return position_; }

    //! convenience method for getting the mouse x position
    float x() const { return position().x(); }

    //! convenience method for getting the mouse y position
    float y() const { return position().y(); }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    Core::Vector2 position_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Message caused by the user clicking a mouse button
class MouseClickMessage : public MouseMessage
{
    // ---------------------------------------------------------------------------------------------
    //  Public enumerations
    // ---------------------------------------------------------------------------------------------
public:
    enum Button { left, middle, right };
    enum Event { pressed, released, doubleClicked };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Constuction
    MouseClickMessage(Id anId, const Core::Vector2& aPosition, Button aButton, Event anEvent);

    //! destructor
    virtual ~MouseClickMessage() {}



    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the mouse button involved in the message
    Button button() const { return button_; }

    //! returns the event which invoked the message
    Event event() const { return event_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    Button button_;
    Event event_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Listener for mouse messages
class MouseListener : public MessageListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor
    MouseListener();

    //! destructor
    virtual ~MouseListener() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! process a click message
    virtual bool clicked(MouseClickMessage& ) { return false; }

    //! process the entered message
    virtual bool entered(MouseMessage& ) { return false; }

    //! process the exited message
    virtual bool exited(MouseMessage& ) { return false; }

    //! process the moved message
    virtual bool moved(MouseMessage& ) { return false; }

    //! \brief see if the message can be interpreted by this class and if so pass the
    //! message on to the recipient method
    virtual bool dispatchMessage(const Message& message, Window* receiverWindow);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! we always remember the old mouse position, so that "deltas" are available
    Core::Vector2 oldPosition_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
