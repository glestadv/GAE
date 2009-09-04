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
#include "mmgr/nommgr.h"

#include <algorithm>
#include <functional>

#include "mmgr/mmgr.h"

#include "glgooey/WindowManager.h"

#include "glgooey/core/OpenGL.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/core/Rectangle.h"

#include "glgooey/Window.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/KeyMessage.h"
#include "glgooey/Timer.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/TimeManager.h"




// **************************************************************************************************
namespace Gooey
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//  static variables
std::auto_ptr<WindowManager> WindowManager::instance_;
const float WindowManager::doubleClickTimeThreshold_ = 0.3f;
const float WindowManager::doubleClickMoveThreshold_ = 5.0f;
std::string WindowManager::defaultFont_;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager::WindowManager() :
    lastClickTime_(0),
    lastClickButton_(-1),
    windowBelowCursor_(0),
    focusedWindow_(0),
    windowCapture_(0),
    popUpWindow_(0),
    propertyScheme_(0),
    shiftFunc_(0),
    altFunc_(0),
    ctrlFunc_(0)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager::~WindowManager()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::initialize(const std::string& aDefaultFontName,
    KeyModifierFunc shiftPressedFunc, KeyModifierFunc altPressedFunc, KeyModifierFunc ctrlPressedFunc)
{
    defaultFont_ = aDefaultFontName;
    shiftFunc_ = shiftPressedFunc;
    altFunc_ = altPressedFunc;
    ctrlFunc_ = ctrlPressedFunc;

    propertyScheme_ = new PropertyScheme;

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const WindowManager&
WindowManager::update() const
{
    triggerElapsedTimers();

    Renderer::preRender();

    // do the rendering
    for(Windows::const_reverse_iterator wit = windows_.rbegin(); wit != windows_.rend(); ++wit)
    {
        const TopLevelWindow& tlw = *wit;
        tlw.window->render();
    }

    if(popUpWindow_ != 0) popUpWindow_->render();


    Renderer::postRender();

    return *this;

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::addWindow(Window* aWindow)
{
    TopLevelWindow tlw(aWindow, largestZValue() + 1);
    windows_.push_back(tlw);

    windows_.sort(std::greater<TopLevelWindow>());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::removeAllWindows()
{
    windowBelowCursor_ = 0;
    focusedWindow_ = 0;
    windows_.clear();

    return*this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::removeWindow(Window* aWindow)
{
    if(isTopLevelWindow(aWindow))
    {
        TopLevelWindow tlw(aWindow, zValueOf(aWindow));
        windows_.remove(tlw);
        if(windowBelowCursor() != 0)
        {
            if(aWindow->doesHierarchyContain(windowBelowCursor()))
                checkForWindowBelowCursor();
        }
        if(focusedWindow() != 0)
        {
            if(aWindow->doesHierarchyContain(focusedWindow()))
                focusedWindow_ = 0;
        }
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::removeTimer(Timer* aTimer)
{
    Timers::iterator it;
    bool isFound = false;
    for(it = timers_.begin(); it != timers_.end(); ++it)
    {
        Timer* timer = *it;
        if(timer == aTimer)
        {
            isFound = true;
            break;
        }
    }

    if(isFound) timers_.erase(it);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::isTopLevelWindow(Window* aWindow) const
{
    bool ret = false;
    for(Windows::const_iterator it = windows_.begin(); it != windows_.end(); ++it)
    {
        if((*it).window == aWindow)
        {
            ret = true;
            break;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const bool
WindowManager::process(Message& message) const
{
    bool ret = false;
    if( (popUpWindow() != 0) && (popUpWindow()->isVisible()) )
    {
        popUpWindow()->process(message);
    }
    else
    {
        for(Windows::const_iterator it = windows_.begin(); it != windows_.end(); ++it)
        {
            ret = ret || (*it).window->process(message);
        }
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::setMouseCapture(Window * window)
{
    ASSERT(windowCapture_ == 0);
    ASSERT(window != 0);

    windowCapture_ = window;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::releaseMouseCapture()
{
    ASSERT(windowCapture_ != 0);

    windowCapture_ = 0;

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::processMouseMessage(MouseClickMessage& msg)
{
    bool isHandled = false;
    if(windowCapture_ != 0)
    {
        isHandled = windowCapture_->process(msg);
    }
    else
    {
        if(windowBelowCursor() != 0)
            isHandled = windowBelowCursor()->process(msg);
        else
            isHandled = process(msg); // XT : standard processing
    }
    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onLeftButtonDown(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);

    MouseClickMessage::Event evt = MouseClickMessage::pressed;
    if( lastClickButton_ == MouseClickMessage::left && isLastClickDoubleClick() )
    {
        evt = MouseClickMessage::doubleClicked;
    }
    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::left, evt);
    bool isHandled = processMouseMessage(rval);

    updateFocusIfNecessary();

    if(evt != MouseClickMessage::doubleClicked)
    {
        lastClickButton_ = MouseClickMessage::left;
        lastClickTime_ = TimeManager::systemTime();
        lastClickPos_ = cursorPos_;
    }
    else
    {
        lastClickButton_ = -1;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onLeftButtonUp(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);
    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::left, MouseClickMessage::released);
    return processMouseMessage(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onMiddleButtonDown(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);

    MouseClickMessage::Event  evt = MouseClickMessage::pressed;
    if( lastClickButton_ == MouseClickMessage::middle && isLastClickDoubleClick() )
    {
        evt = MouseClickMessage::doubleClicked;
    }

    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::middle, evt);
    bool isHandled = processMouseMessage(rval);

    if( evt != MouseClickMessage::doubleClicked )
    {
        lastClickButton_ = MouseClickMessage::middle;
        lastClickTime_ = TimeManager::systemTime();
        lastClickPos_ = cursorPos_;
    }
    else
    {
        lastClickButton_ = -1;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onMiddleButtonUp(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);
    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::middle, MouseClickMessage::released);
    return processMouseMessage(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onRightButtonDown(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);

    Gooey::MouseClickMessage::Event evt = MouseClickMessage::pressed;
    if( lastClickButton_ == MouseClickMessage::right && isLastClickDoubleClick() )
    {
        evt = MouseClickMessage::doubleClicked;
    }

    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::right, evt);
    bool isHandled = processMouseMessage(rval);

    if (evt != MouseClickMessage::doubleClicked)
    {
        lastClickButton_ = MouseClickMessage::right;
        lastClickTime_ = TimeManager::systemTime();
        lastClickPos_ = cursorPos_;
    }
    else
    {
        lastClickButton_ = -1;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onRightButtonUp(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);
    checkForWindowBelowCursor();
    MouseClickMessage rval(Message::mouseClick, Core::Vector2(x, y), MouseClickMessage::right, MouseClickMessage::released);
    return processMouseMessage(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onMouseMove(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    cursorPos_ = Core::Vector2(x, y);
    checkForWindowBelowCursor();
    MouseMessage rval(Message::mouseMoved, Core::Vector2(x, y));
    bool isHandled = false;
    if(windowCapture_)
    {
        isHandled = windowCapture_->process(rval);
    }
    else
    {
        if(windowBelowCursor() != 0)
            isHandled = /*windowBelowCursor()->*/process(rval);
        else
            isHandled = process(rval); // XT : standard processing
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onChar(unsigned char charCode)
{
    KeyMessage rval(Message::characterInput, charCode, 0);
    return process(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onKeyDown(int keyCode)
{
    KeyMessage rval(Message::keyPressed, 0, static_cast<unsigned short>(keyCode));
    return process(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::onKeyUp(int keyCode)
{
    KeyMessage rval(Message::keyReleased, 0, static_cast<unsigned short>(keyCode));
    return process(rval);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::instance()
{
    if(instance_.get() == 0)
    {
        std::auto_ptr<WindowManager> tempAutoPtr(new WindowManager);
        instance_ = tempAutoPtr;
    }
    return *instance_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::checkForWindowBelowCursor()
{
    // update the window below the cursor
    Windows::const_iterator it;
    bool done = false;
    if(popUpWindow_ != 0)
    {
        if(popUpWindow_->doesHierarchyContain(popUpWindow_->windowBelow(cursorPos_)))
        {
            windowBelowCursor_ = popUpWindow_;
            done = true;
        }
    }

    if(!done)
    {
        Window* wbc = 0;
        for(it = windows_.begin(); it != windows_.end(); ++it)
        {
            Window* windowToTest = (*it).window;

            if(windowToTest->isVisible())
            {
                Window* w = windowToTest->windowBelow(cursorPos_);
                if(w != 0)
                {
                    wbc = w;
                    break;
                }
            }
        }
        windowBelowCursor_ = wbc;
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
WindowManager::terminate()
{
    delete propertyScheme_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int
WindowManager::zValueOf(Window* aWindow) const
{
    if(!isTopLevelWindow(aWindow))
        THROW("Trying to get the zValue of a window that is not a top level window");

    int ret = 0;
    for(Windows::const_iterator it = windows_.begin(); it != windows_.end(); ++it)
    {
        const TopLevelWindow& tlw = *it;
        if(tlw.window == aWindow)
        {
            ret = tlw.zVal;
            break;
        }
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int
WindowManager::largestZValue() const
{
    int ret = 0;
    if(windows_.size() > 0)
    {
        Windows::const_iterator it = windows_.begin();
        ret = (*it).zVal;
        while(it != windows_.end())
        {
            const TopLevelWindow& tlw = *it;
            if(tlw.zVal > ret) ret = tlw.zVal;
            ++it;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::setActiveWindow(Window* aWindow)
{
    Window* topLevelWindow = aWindow->topLevelWindow();
    if(isTopLevelWindow(topLevelWindow))
    {
        int zVal = zValueOf(topLevelWindow);
        Windows::iterator it = std::find(windows_.begin(), windows_.end(), TopLevelWindow(aWindow, zVal));
        if ( it != windows_.end() )
        {
            TopLevelWindow& tlw = *it;
            tlw.zVal = largestZValue() + 1;
        }
    }

    windows_.sort(std::greater<TopLevelWindow>());

    int z = 0;
    for(Windows::reverse_iterator it = windows_.rbegin(); it != windows_.rend(); ++it)
    {
        TopLevelWindow& tlw = *it;
        tlw.zVal = z;
        z++;
    }

    checkForWindowBelowCursor();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::isShiftPressed() const
{
    ASSERT ( shiftFunc_ != 0 );

    return (shiftFunc_)();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::isAltPressed() const
{
    ASSERT ( altFunc_ != 0 );

    return (altFunc_)();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::isCtrlPressed() const
{
    ASSERT ( ctrlFunc_ != 0 );

    return (ctrlFunc_)();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::loadPropertyScheme(const std::string& aFileName)
{
    delete propertyScheme_;
    propertyScheme_ = new PropertyScheme;
    propertyScheme_->load(aFileName);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::copyToClipboard(const std::string& str)
{
    clipboard_ = str;
    clipboardContentsChanged(str);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
WindowManager::clipboardContents() const
{
    return clipboard_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window*
WindowManager::popUpWindow() const
{
    return popUpWindow_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
WindowManager&
WindowManager::applicationResized(int anX, int aY)
{
    const float x = static_cast<float>(anX);
    const float y = static_cast<float>(aY);
    applicationSize_ = Core::Vector2(x, y);

    for(Windows::const_iterator it = windows_.begin(); it != windows_.end(); ++it)
    {
        const Dock& dock = (*it).dock;
        if(dock.docked())
        {
            Window* aWindow = (*it).window;
            Core::Vector2 pos = aWindow->position();
            dock.computePosition(pos,aWindow->size(),Core::Vector2(x,y));
            aWindow->setPosition(pos);
        }
        else
        {
            Window* aWindow = (*it).window;
            Core::Vector2 pos = aWindow->position();
            aWindow->setPosition(pos);
        }
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
WindowManager::setDock(Window* aWindow,const Dock& d)
{
    if(!isTopLevelWindow(aWindow))
        return;

    Windows::iterator it = std::find(windows_.begin(), windows_.end(), TopLevelWindow(aWindow, 0));
    if ( it == windows_.end() )
        return;

    (*it).dock = d;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
WindowManager::isLastClickDoubleClick() const
{
    return ( (TimeManager::systemTime() - lastClickTime_ < doubleClickTimeThreshold_) &&
        (Core::abs(lastClickPos_.x()-cursorPos_.x()) < doubleClickMoveThreshold_) &&
        (Core::abs(lastClickPos_.y()-cursorPos_.y()) < doubleClickMoveThreshold_) );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
WindowManager::updateFocusIfNecessary()
{
    if( (popUpWindow() == 0) || (!popUpWindow()->windowRectangle().surrounds(cursorPos_)) )
    {
        if( windowBelowCursor() != focusedWindow() )
        {
            setFocusedWindow(windowBelowCursor());
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
WindowManager::setFocusedWindow(Window* aWindow)
{
    if(aWindow != focusedWindow())
    {
        if(focusedWindow() != 0) focusedWindow()->disableFocused();
        focusedWindow_ = aWindow;
        if(aWindow != 0) aWindow->enableFocused();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
WindowManager::triggerElapsedTimers() const
{
    Timers::const_iterator it;
    for(it = timers_.begin(); it != timers_.end(); ++it)
    {
        Timer* timer = *it;
        if(timer->hasElapsed())
        {
            timer->trigger();
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
} // namespace Gooey

