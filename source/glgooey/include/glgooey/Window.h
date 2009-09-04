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
#ifndef WINDOW__H
#define WINDOW__H

#include "mmgr/mmgr.h"

#include "glgooey/core/Serialization.h"
#include "glgooey/core/Vector2.h"

#include "sigslot.h"
#include "glgooey/Color.h"


// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    namespace Core
    {
        class Rectangle;
    }

    class MessageListener;
    class Message;
}
// *************************************************************************************************



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Base class for all Gooey window elements
//!
//!  All windows maintain a list of message listeners. These are attached to the window by
//! calling the addMessageListener() method. When an message is sent to a window, the list of
//! attached message listeners is checked to see if there is a listener which can deal with the
//! incoming message
class Window : public SlotHolder
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! Linked list of message listeners
    typedef std::list<MessageListener*> MessageListeners;


    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief default constructor
    //! \param aParent the parent window
    //! \param aText the window text
    explicit Window(Window* aParent = 0, const std::string& aText = "");

    //! destructor
    virtual ~Window();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Returns the window text;
    //!
    //! The window text is a string with a context sensitive interpretation. It can be the
    //! label of a button or the title of a frame window etc.
    std::string text() const { return text_; }

    //! \brief Sets the window text; a string with a context sensitive
    //! interpretation. It can be the label of a button or the title of a
    //! frame window etc.
    Window& setText(const std::string& aText);

    //! returns the dimensions of the window in pixels
    Core::Vector2 size() const;

    //! sets the dimensions of the window in pixels
    Window& setSize(const Core::Vector2& aSize);

    //! returns the position of the window in app window coords
    Core::Vector2 position() const { return position_; }

    //! sets the position of the window in app window coords
    Window& setPosition(const Core::Vector2& aPosition);

    //! returns \c true if this window currently has the focus
    bool isFocused() const;

    //! sets the focus to this window
    Window& enableFocused();

    //! removes the focus from this window
    Window& disableFocused();

    //! Returns \c true iff the window currently has the focus
    virtual bool containsFocus() const { return isFocused(); }



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! render the window to the current OpenGL context
    Window& render();

    //! set the size and position of the window using a rectangle
    Window& setWindowRectangle(const Core::Rectangle& rect);

    //! return the size and position of the window using a rectangle
    Core::Rectangle windowRectangle() const;

    //! add an message listener to the window's list of listeners
    Window& addMessageListener(MessageListener* listener);

    //! remove an message listener from the window's list of listeners
    Window& removeMessageListener(MessageListener* listener);

    //! makes the window visible \sa hide
    Window& show();

    //! makes the window invisible \sa show
    Window& hide();

    //! enable the window
    Window& enable() { isEnabled_ = true; return*this; }

    //! disable the window
    Window& disable() { isEnabled_ = false; return*this; }

    //! returns true iff the window is enabled
    bool isEnabled() const { return isEnabled_; }

    //! returns true iff the window is currently visible
    bool isVisible() const;

    //! returns the recommended dimensions of the window in pixels
    virtual Core::Vector2 recommendedSize() const = 0;

    //! \brief Make the window attempt to process the passed in message.
    //!
    //! Derived classes that contain further windows must override this
    //! to pass the message on to the contained windows
    virtual bool process(const Message& message);

    //! \brief Returns a pointer to the window below the passed in point
    //!
    //! The window manager calls this on top level windows to determine
    //! which window is currently below the mouse cursor. Each derived class
    //! which has child windows must overwrite this and pass the call on to
    //! the children.
    virtual Window* windowBelow(const Core::Vector2& p);

    //! \brief Returns true if the passed in window pointer points to either
    //! this window or a child window of this window.
    //!
    //! Each derived class that has child windows must overwrite this and
    //! pass the call on to the children.
    virtual bool doesHierarchyContain(Window* aWindow) const;

    //! returns the top level window belonging to this window
    Window* topLevelWindow() const;

    //! returns a pointer to this window's parent
    Window* parent() const { return parent_; }

    //! activates the window bringing it to the front of the display
    Window& setAsActiveWindow();

    //! Loads a descriptor for this individual window instance from an xml file
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName) = 0;


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    Signal0 closed; //!< Signal used for notification of the window closing


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw() = 0;

    //! \brief called when the window is resized, derived classes can override this to
    //! react to the window size changing
    virtual void onResize() {}

    //! \brief called when the window is moved, derived classes can override this to
    //! react to the window position changing
    virtual void onMove() {}

    //! \brief called when the window text changes, derived classes can override this to
    //! react to the window text changing
    virtual void onTextChanged() {}

    //! \brief called when the window is shown, derived classes can override this to
    //! react to the window becoming visible
    virtual void onShow() {}

    //! \brief called when the window is hidden, derived classes can override this to
    //! react to the window being hidden
    virtual void onHide() {}

    //! Called when the window receives the focus
    virtual void onGainedFocus() {}

    //! Called when the window loses the focus
    virtual void onLostFocus() {}

private:
    Window* parent_;  //!< The window's parent (or 0 if this window has no parent)

    MessageListeners messageListeners_; //!< The message listeners attached to this window

    std::string text_;          //!< The window text
    Core::Vector2 size_;        //!< The size of the window
    Core::Vector2 position_;    //!< The position of the window
    bool isVisible_;            //!< The visiblility flag
    bool isEnabled_;            //!< Whether the window is currently enabled
    bool isSizeSetByUser_;      //!< Whether the user has explicitly set the window's size
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
