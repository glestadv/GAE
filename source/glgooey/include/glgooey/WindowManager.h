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
#ifndef WINDOWMANAGER__H
#define WINDOWMANAGER__H

#include "mmgr/mmgr.h"

#include "glgooey/Color.h"
#include "glgooey/core/Vector2.h"
#include "Dock.h"
#include "sigslot.h"




// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class Window;
    class Timer;
    class Message;
    class PropertyScheme;
    class MouseClickMessage;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{

//! Typedef for the type of function to be called when finding out whether a modifier key is pressed
typedef bool (*KeyModifierFunc)();




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The "manager" for the Gooey system
//!
//!   Acts as a facade between Gooey and whatever window
//! system Gooey happens to be running on. Here a list
//! of top level windows is maintained. Methods for reacting
//! to messages are provided. Message translating is also
//! processd by the window manager.
//!
//! \note This class is a singleton. It can only be instantiated via the
//!       instance() method.
class WindowManager
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
private:
    //! An internal structure which maintains information on top level windows
    struct TopLevelWindow
    {
        Window* window;
        int     zVal;
        Dock    dock;
        TopLevelWindow(Window* aWindow, int aZVal,const Dock* d=0) : window(aWindow), zVal(aZVal)
        { if (d) dock=*d; }
        TopLevelWindow(const TopLevelWindow& tlw) { (*this) = tlw; }
        TopLevelWindow& operator = (const TopLevelWindow& tlw)
        { window = tlw.window; zVal = tlw.zVal; dock=tlw.dock; return *this; }
        bool operator == (const TopLevelWindow& tlw) const
        { return ((window == tlw.window) /*&& (zVal == tlw.zVal)*/); } // Modif SB
        bool operator != (const TopLevelWindow& tlw) const
        { return !((*this) == tlw); }
        bool operator > (const TopLevelWindow& tlw) const
        { return (zVal > tlw.zVal); }
    };
    typedef std::list<TopLevelWindow>  Windows;
    typedef std::list<Timer*>          Timers;



    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
private:
    //! default constructor
    WindowManager();

public:
    //! destructor
    virtual ~WindowManager();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns a pointer to the window that is currently below the mouse cursor
    Window* windowBelowCursor() const { return windowBelowCursor_; }

    //! Returns a pointer to the window that was previously below the mouse cursor
    Window* oldWindowBelowCursor() const { return oldWindowBelowCursor_; }

    //! returns a pointer to the window that currently has the focus
    Window* focusedWindow() const { return focusedWindow_; }

    //! Sets the focused window
    void setFocusedWindow(Window* aWindow);

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns a reference to the only instance of this class
    static WindowManager& instance();

    //! \brief Initializes the GLGooey window manager
    //!
    //! This must be called once before any other Gooey stuff is done The typical GLGooey
    //! initialization will look something like this:
    //! \code
    //! WindowManager::instance().initialize("./data/myFont.ttf", shiftPressed, altPressed, ctrlPressed);
    //! \endcode
    //! where the xxxPressed parameters are functions such as
    //! \code
    //! bool shiftPressed()
    //! {
    //!     // return true if the shift key is pressed
    //! }
    //! \endcode
    //! \param aDefaultFontName the name of a font to be used when no other font name is specified
    //! \param shiftPressedFunc a function which returns true if the SHIFT key is pressed
    //! \param altPressedFunc a function which returns true if the ALT key is pressed
    //! \param ctrlPressedFunc a function which returns true if the CTRL key is pressed
    WindowManager& initialize(const std::string& aDefaultFontName, KeyModifierFunc shiftPressedFunc,
        KeyModifierFunc altPressedFunc, KeyModifierFunc ctrlPressedFunc);

    //! this is called to do any cleaning up once the app has terminated
    void terminate();

    //! Add a top level window to the window manager
    WindowManager& addWindow(Window* aWindow);

    //! Returns a pointer to the top popUp window if there is one, otherwise 0
    Window* popUpWindow() const;

    //! Remove all top level windows from the window manager without deleting them!!
    WindowManager& removeAllWindows();

    //! Remove a particluar window from the window manager without deleting it!!
    WindowManager& removeWindow(Window* aWindow);

    //!  Returns true if the passed in window is a top level window in the
    //! window manager's list
    bool isTopLevelWindow(Window* aWindow) const;

    //! Add a timer to the window manager
    WindowManager& addTimer(Timer* aTimer) { timers_.push_back(aTimer); return*this; }

    //! Remove a timer from the window manager
    WindowManager& removeTimer(Timer* aTimer);

    //! Checks to see which window is below the cursor - get the result by calling windowBelowCursor()
    WindowManager& checkForWindowBelowCursor();

    //! Checks timers and renders the Gooey windows
    const WindowManager& update() const;

    //! Set mouse Capture to the given window
    //! \author XT
    WindowManager& setMouseCapture(Window* window);

    //! Unset mouse Capture
    //! \author XT
    WindowManager& releaseMouseCapture();

    //! Get the capture window
    //! \author XT
    Window* captureWindow() const { return windowCapture_; }

    //! \brief Makes the passed in window's top level window the active window. It will be
    //! drawn infront of all other windows
    WindowManager& setActiveWindow(Window* aWindow);

    //! the application's window was resized
    WindowManager& applicationResized(int x, int y);

    //! returns a reference to the currently used property scheme
    PropertyScheme& propertyScheme() { return *propertyScheme_; }

    //! \brief returns whether the shift key is currently pressed. This calls a
    //! callback which MUST be set by the application on initialization
    //!
    //! \sa initialize
    bool isShiftPressed() const;

    //! \brief returns whether the alt key is currently pressed. This calls a
    //! callback which MUST be set by the application on initialization
    //!
    //! \sa initialize
    bool isAltPressed() const;

    //! \brief returns whether the ctrl key is currently pressed. This calls a
    //! callback which MUST be set by the application on initialization
    //!
    //! \sa initialize
    bool isCtrlPressed() const;

    //! sets the clipboard string
    WindowManager& copyToClipboard(const std::string& str);

    //! returns the clipboard string
    std::string clipboardContents() const;

    //! Returns the size of the application window within which GLGooey is running
    Core::Vector2 applicationSize() const { return applicationSize_; }

    //! returns the name of the default font to be used when no other font is specified
    static std::string defaultFont() { return defaultFont_; }

    //! sets a control to be regarded as popUp
    WindowManager& setPopUpWindow(Window* aWindow)
    {
        popUpWindow_ = aWindow;
        return *this;
    }

    //! loads a new property scheme from the file with the passed in name
    WindowManager& loadPropertyScheme(const std::string& aFileName);

    // ---------------------------------------------------------------------------------------------
    //   Message Handling
    //  call these from the host app to integrate Gooey. The boolean value which
    //  is returned indicates whether the passed in message was handled by
    //  a window withing GLGooey or not.
    // ---------------------------------------------------------------------------------------------
public:
    //! left mouse button pressed
    bool onLeftButtonDown  (int x, int y);

    //! left mouse button released
    bool onLeftButtonUp    (int x, int y);

    //! middle mouse button pressed
    bool onMiddleButtonDown(int x, int y);

    //! middle mouse button released
    bool onMiddleButtonUp  (int x, int y);

    //! right mouse button pressed
    bool onRightButtonDown (int x, int y);

    //! right mouse button released
    bool onRightButtonUp   (int x, int y);

    //! mouse moved
    bool onMouseMove       (int x, int y);

    //! keyboard character entered
    bool onChar    (unsigned char charCode);

    //! keyboard key was pressed
    bool onKeyDown (int  keyCode);

    //! keyboard key was released
    bool onKeyUp   (int  keyCode);

    void setDock(Window* aWindow,const Dock& d);



    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! emitted when the clipboard contents change
    Signal1<const std::string&> clipboardContentsChanged;



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    //! pass the message on to all windows maintained by the manager for handling
    const bool process(Message& message) const;

    //! pointer to the only instance of this class
    static std::auto_ptr<WindowManager> instance_;

    //! The name of the default font, used when no other font is specified
    static std::string defaultFont_;

    //! helper for handling mouse messages
    bool processMouseMessage(MouseClickMessage& msg);

    //! Returns true if the conditions for a double click were met on the last click
    bool isLastClickDoubleClick() const;

    //! Sees if the focus has changed and updates the system if necessary
    void updateFocusIfNecessary();

    //! Triggers any timers that have elapsed
    void triggerElapsedTimers() const;


    Windows windows_;  //!< the windows maintained by this class

    Timers timers_;   //!< the currently active timers

    Core::Vector2 applicationSize_; //!< the size of the application's window we're running in

    Core::Vector2 cursorPos_; //!< the current position of the mouse cursor

    Core::Vector2 lastClickPos_; //!< the location of the last mouse click

    float lastClickTime_; //!< the time of the last mouse click

    int lastClickButton_; //!< the last mouse button clicked

    Window* windowBelowCursor_;    //!< A pointer to the window that is currently below the cursor
    Window* oldWindowBelowCursor_; //!< A pointer to the window that was previously below the cursor
    Window* focusedWindow_;        //!< A pointer to the window that currently has the input focus
    Window* windowCapture_;        //!< A pointer to the window that has currently captured the mouse input

    Window* popUpWindow_;   //!< A pop-up window (only one can be open at a time)

    PropertyScheme* propertyScheme_;  //!< The property scheme being used to draw windows

    int zValueOf(Window* aWindow) const; //!< Returns the z value of the passed in window

    int largestZValue() const; //!< Returns the largest z value of all windows

    std::string clipboard_; //!< The string used for copying and pasting

    static const float doubleClickTimeThreshold_; //!< The time within which two clicks are considered a double-click
    static const float doubleClickMoveThreshold_; //!< The distance within which two clicks are considered a double-click

    KeyModifierFunc shiftFunc_;  //!< A pointer to the function used to establish whether the shift key is pressed
    KeyModifierFunc altFunc_;    //!< A pointer to the function used to establish whether the alt key is pressed
    KeyModifierFunc ctrlFunc_;   //!< A pointer to the function used to establish whether the ctrl key is pressed
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
