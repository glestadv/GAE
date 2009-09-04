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
#ifndef FRAMEWINDOW__H
#define FRAMEWINDOW__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/RenderDesc.h"

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    namespace Core
    {
        class Rectangle;
    }
    class FrameWindowMouseListener;
    class FrameWindowDesc;
    class TextDesc;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Window with title bar, border and client area
class FrameWindow : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor
    FrameWindow(const Core::Rectangle& rect, Window* aParent = 0, const std::string& aText = "");

    //! destructor
    virtual ~FrameWindow();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the frame window's client window
    Window* clientWindow() const { return clientWindow_; }

    //! Sets the frame window's client window
    FrameWindow& setClientWindow(Window* aWindow)
    {
        clientWindow_ = aWindow;
        clientWindowChanged();
        return *this;
    }

    //! Returns true if the window can be moved
    bool isMovable() const;

    //! Make the window moveable (hold the left mouse down on the window frame and drag)
    FrameWindow& enableMovement();

    //! Prevent the window from being moved
    FrameWindow& disableMovement();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! return the rectangle in which the client resides
    Core::Rectangle clientRectangle() const;

    //! make the window attempt to process the passed in message
    virtual bool process(const Message& message);

    //! \brief the window manager calls this on top level windows to determine
    //! which window is currently below the mouse cursor.
    //! The frame window overwrites this to pass the call on to its client
    //! window.
    virtual Window* windowBelow(const Core::Vector2& p);

    //! \brief returns true if the passed in window pointer points to either
    //! this window or a child window of this window.
    virtual bool doesHierarchyContain(Window* aWindow) const;

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! react to the client window changing
    virtual FrameWindow& clientWindowChanged();

    //! called when the window is resized (resizes the client)
    virtual void onResize();

    //! called when the window is moved (repositions the client)
    virtual void onMove();

private:
    FrameWindowMouseListener* mouseListener_;
    Window* clientWindow_;
    int captureCount_;
    bool isMovable_;

    friend class FrameWindowMouseListener;

    FrameWindowDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for frame windows
class FrameWindowDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    FrameWindowDesc();

    //! Destructor
    virtual ~FrameWindowDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! sets the frame window that this desc will be drawing
    FrameWindowDesc& setFrameWindow(FrameWindow* aFrameWindow)
    {
        frameWindow_ = aFrameWindow;
        return *this;
    }

    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns the border width at the top of the frame
    float topBorderWidth() const { return topBorderWidth_; }

    //! Returns the border width at the left of the frame
    float leftBorderWidth() const { return leftBorderWidth_; }

    //! Returns the border width at the right of the frame
    float rightBorderWidth() const { return rightBorderWidth_; }

    //! Returns the border width at the bottom of the frame
    float bottomBorderWidth() const { return bottomBorderWidth_; }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    RenderDesc*  topLeftDesc_;
    RenderDesc*  titleDesc_;
    RenderDesc*  topDesc_;
    RenderDesc*  topRightDesc_;
    RenderDesc*  rightDesc_;
    RenderDesc*  bottomRightDesc_;
    RenderDesc*  bottomDesc_;
    RenderDesc*  bottomLeftDesc_;
    RenderDesc*  leftDesc_;
    TextDesc*    textDesc_;

    float topBorderWidth_;
    float leftBorderWidth_;
    float rightBorderWidth_;
    float bottomBorderWidth_;

    FrameWindow* frameWindow_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(topLeftDesc) &
            TAGGED_OBJECT(titleDesc) &
            TAGGED_OBJECT(topDesc) &
            TAGGED_OBJECT(topRightDesc) &
            TAGGED_OBJECT(rightDesc) &
            TAGGED_OBJECT(bottomRightDesc) &
            TAGGED_OBJECT(bottomDesc) &
            TAGGED_OBJECT(bottomLeftDesc) &
            TAGGED_OBJECT(leftDesc) &
            TAGGED_OBJECT(textDesc) &
            TAGGED_OBJECT(topBorderWidth) &
            TAGGED_OBJECT(leftBorderWidth) &
            TAGGED_OBJECT(rightBorderWidth) &
            TAGGED_OBJECT(bottomBorderWidth);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
