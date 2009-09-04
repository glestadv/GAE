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
#ifndef PANEL__H
#define PANEL__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/BaseLayouter.h"
#include "glgooey/SolidSurfaceDesc.h"


// *************************************************************************************************
//  Forward Declarations
namespace Gooey
{
    class PanelDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief a plain rectangular panel
//!
//! This is a child window with no functionality save to contain further child windows. For
//! instance the client area of a frame window can be filled with a panel. Further windows can
//! then be added to the panel.
class Panel : public Window
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! \brief Constructs a Panel object
    //!
    //! Construction requires the rectangle which the panel fills and a parent.
    //! Window text may be set, but is not visible in the GUI
    Panel(const Core::Rectangle& rectangle, Window* aParent, const std::string& aText = "");

    //! destructor
    virtual ~Panel();


    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! Adds a child window to the panel
    Panel& addChildWindow(Window* aWindow)
    {
        windows_.push_back(aWindow);
        layouter_->addWindow(aWindow);
        return *this;
    }

    //! Adds a child window to the panel using additional layout info
    Panel& addChildWindow(Window* aWindow, const LayoutInfo& info)
    {
        windows_.push_back(aWindow);
        layouter_->addWindow(aWindow, info);
        return *this;
    }

    //! Removes a child window from the panel
    Panel& removeChildWindow(Window* aWindow)
    {
        windows_.remove(aWindow);
        layouter_->removeWindow(aWindow);
        return *this;
    }

    //! Gets the layouter to arrange the windows
    Panel& arrangeChildren()
    {
        layouter_->arrangeWithin(this);
        return *this;
    }

    //! Use this method to set a new layouter
    Panel& setLayouter(BaseLayouter* aLayouter);

    //! \brief Make the panel attempt to process the passed in message
    //! if necessary the message is passed on to relevant child windows
    virtual bool process(const Message& message);

    //! \brief See which window is below the mouse cursor
    //!
    //!  The window manager calls this on top level windows to determine
    //! which window is currently below the mouse cursor. The panel overrides
    //! this to pass the call on to its child windows.
    virtual Window* windowBelow(const Core::Vector2& p);

    //! \brief Returns true if the passed in window pointer points to either
    //! this window or a child window of this window.
    virtual bool doesHierarchyContain(Window* aWindow) const;

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);



    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! called when the window is resized (rearranges the children)
    virtual void onResize();

    //! called when the window is moved (rearranges the children)
    virtual void onMove();

private:
    BaseLayouter* layouter_;
    std::list<Window*> windows_;
    PanelDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The RenderDesc class for a panel
class PanelDesc : public RenderDesc
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    PanelDesc() :
        background_(new SolidSurfaceDesc(Color(0.2f, 0.2f, 0.4f, 0.75f))),
        border_(new RectangleDesc()) {}

    //! Destructor
    virtual ~PanelDesc()
    {
        delete background_;
        delete border_;
    }

    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    RenderDesc* background_;
    RenderDesc* border_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(background) & TAGGED_OBJECT(border);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Gooey


#endif
