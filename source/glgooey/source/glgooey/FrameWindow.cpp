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

#include "glgooey/FrameWindow.h"

#include "glgooey/Renderer.h"
#include "glgooey/WindowManager.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/SolidSurfaceDesc.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"



// *************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(FrameWindowDesc);


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class FrameWindowMouseListener : public MouseListener
{

    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires a reference to the associated frame window
    FrameWindowMouseListener(FrameWindow& aFrameWindow) : frameWindow_(aFrameWindow) {}

    //! destructor
    virtual ~FrameWindowMouseListener() {}


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! process the moved message by moving the window if it is currently pressed
    virtual bool moved(MouseMessage& message);

    //! process the click message by moving the window if it is currently pressed
    virtual bool clicked(MouseClickMessage& message);

private:
    FrameWindow& frameWindow_;
    Core::Vector2 _oldMousePos;

    // Copying and assignment are not allowed
    FrameWindowMouseListener(const FrameWindowMouseListener& aFrameWindowMouseListener);
    FrameWindowMouseListener& operator = (const FrameWindowMouseListener& aFrameWindowMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
//                                            FrameWindow
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindow::FrameWindow(const Core::Rectangle& rect, Window* aParent,
                        const std::string& aText) :
    Window(aParent, aText),
    clientWindow_(0),
    captureCount_(0),
    isMovable_(false),
    renderDesc_(0)
{
    // set the geometry
    setWindowRectangle(rect);

    mouseListener_ = new FrameWindowMouseListener(*this);
    addMessageListener(mouseListener_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindow::~FrameWindow()
{
    removeMessageListener(mouseListener_);
    delete mouseListener_;

    delete renderDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Rectangle
FrameWindow::clientRectangle() const
{
    FrameWindowDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().frameWindowDesc_;

    const Core::Vector2 topLeft = position() +
        Core::Vector2(renderDesc->leftBorderWidth(), renderDesc->topBorderWidth());
    const Core::Vector2 bottomRight = position() + size() -
        Core::Vector2(renderDesc->rightBorderWidth(), renderDesc->bottomBorderWidth());
    return Core::Rectangle(topLeft, bottomRight);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindow&
FrameWindow::clientWindowChanged()
{
    if(clientWindow() != 0)
        clientWindow()->setWindowRectangle(clientRectangle());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
FrameWindow::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
    {
        if(clientWindow() != 0)
            ret = clientWindow()->process(message);
        if(!ret) ret = Window::process(message);
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
FrameWindow::doesHierarchyContain(Window* aWindow) const
{
    bool ret = false;
    if(clientWindow() != 0)
        ret = clientWindow()->doesHierarchyContain(aWindow);
    return (ret || Window::doesHierarchyContain(aWindow));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
FrameWindow::recommendedSize() const
{
    return Core::Vector2(200, 200);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
FrameWindow::draw()
{
    FrameWindowDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().frameWindowDesc_;

    renderDesc->setFrameWindow(this);

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();

    renderDesc->draw(Core::Rectangle(topLeft, bottomRight));

    if(clientWindow() != 0)
    {
        Renderer::enableClipRectangle(clientRectangle());
        clientWindow()->render();
        Renderer::disableClipRectangle();
    }


    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window*
FrameWindow::windowBelow(const Core::Vector2& p)
{
    Window* ret = Window::windowBelow(p);
    if(ret == (Window*)(this) && clientWindow())
    {
        Window* cw = clientWindow()->windowBelow(p);
        if(cw != 0) ret = cw;
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
FrameWindow::isMovable() const
{
    return isMovable_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindow&
FrameWindow::enableMovement()
{
    isMovable_ = true;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindow&
FrameWindow::disableMovement()
{
    isMovable_ = false;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
FrameWindow::onResize()
{
    if(clientWindow() != 0)
        clientWindow()->setWindowRectangle(clientRectangle());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
FrameWindow::onMove()
{
    if(clientWindow() != 0)
        clientWindow()->setWindowRectangle(clientRectangle());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
FrameWindow::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new FrameWindowDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<FrameWindowDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// **************************************************************************************************
//                                        FrameWindowMouseListener
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
FrameWindowMouseListener::moved(MouseMessage& message)
{
    bool isHandled = false;
    if(frameWindow_.isMovable())
    {
        if(WindowManager::instance().captureWindow() == &frameWindow_)
        {
            Core::Vector2 delta(message.x() - _oldMousePos.x(), message.y() - _oldMousePos.y());
            frameWindow_.setPosition(frameWindow_.position() + delta);
        }
        _oldMousePos.setX(message.x());
        _oldMousePos.setY(message.y());
        isHandled = true;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
FrameWindowMouseListener::clicked(MouseClickMessage& message)
{
    bool isHandled = false;
    if(message.button() == MouseClickMessage::left)
    {
        if(message.event() == MouseClickMessage::pressed)
        {
            if(frameWindow_.isMovable())
            {
                if(WindowManager::instance().captureWindow() != &frameWindow_)
                    WindowManager::instance().setMouseCapture(&frameWindow_);
                frameWindow_.captureCount_++;
            }
            frameWindow_.setAsActiveWindow();
            isHandled = true;
        }
        else if(message.event() == MouseClickMessage::released)
        {
            if(frameWindow_.isMovable())
            {
                if(frameWindow_.captureCount_ == 1)
                    WindowManager::instance().releaseMouseCapture();
                if(frameWindow_.captureCount_ > 0)
                    frameWindow_.captureCount_--;
            }
            isHandled = true;
        }
    }
    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                            FrameWindowDesc
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindowDesc::FrameWindowDesc() :
    topLeftDesc_(new DefaultFrameDesc(topLeft)),
    titleDesc_(new DefaultFrameDesc(title)),
    topDesc_(new DefaultFrameDesc(top)),
    topRightDesc_(new DefaultFrameDesc(topRight)),
    rightDesc_(new DefaultFrameDesc(right)),
    bottomRightDesc_(new DefaultFrameDesc(bottomRight)),
    bottomDesc_(new DefaultFrameDesc(bottom)),
    bottomLeftDesc_(new DefaultFrameDesc(bottomLeft)),
    leftDesc_(new DefaultFrameDesc(left)),
    textDesc_(new TextDesc(WindowManager::defaultFont(), 18, 0, 0, 0, Color(0.0f, 0.0f, 0.3f, 1.0f))),
    topBorderWidth_(30.0f),
    leftBorderWidth_(30.0f),
    rightBorderWidth_(30.0f),
    bottomBorderWidth_(30.0f),
    frameWindow_(0)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FrameWindowDesc::~FrameWindowDesc()
{
    delete topLeftDesc_;
    delete titleDesc_;
    delete topDesc_;
    delete topRightDesc_;
    delete rightDesc_;
    delete bottomRightDesc_;
    delete bottomDesc_;
    delete bottomLeftDesc_;
    delete leftDesc_;
    delete textDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
FrameWindowDesc::draw(const Core::Rectangle& area) const
{
    Renderer::setCurrentColor(Color(1, 1, 1, 1));

    // ***********************************************************************
    //  draw the background
    // ***********************************************************************

    const Core::Vector2 topLeft = frameWindow_->position();
    const Core::Vector2 bottomRight = frameWindow_->position() + frameWindow_->size();
    const Core::Vector2 topRight(bottomRight.x(), topLeft.y());
    const Core::Vector2 bottomLeft(topLeft.x(), bottomRight.y());


    // ***********************************************************************
    //  draw the border
    // ***********************************************************************

    // get the centers of the four rounded corners
    const Core::Vector2 topLeftCenter     = topLeft + Core::Vector2(leftBorderWidth_, topBorderWidth_);
    const Core::Vector2 bottomRightCenter = bottomRight + Core::Vector2(-rightBorderWidth_, -bottomBorderWidth_);
    const Core::Vector2 bottomLeftCenter  = Core::Vector2(topLeft.x() + leftBorderWidth_, bottomRight.y() - bottomBorderWidth_);
    const Core::Vector2 topRightCenter    = Core::Vector2(bottomRight.x() - rightBorderWidth_, topLeft.y() + topBorderWidth_);

    topLeftDesc_->draw(Core::Rectangle(topLeft, topLeftCenter));
    topRightDesc_->draw(Core::Rectangle(topRight, topRightCenter));
    rightDesc_->draw(Core::Rectangle(topRightCenter, Core::Vector2(bottomRight.x(), bottomRightCenter.y())));
    bottomRightDesc_->draw(Core::Rectangle(bottomRightCenter, bottomRight));
    bottomDesc_->draw(Core::Rectangle(bottomLeftCenter, Core::Vector2(bottomRightCenter.x(), bottomRight.y())));
    bottomLeftDesc_->draw(Core::Rectangle(bottomLeftCenter, bottomLeft));
    leftDesc_->draw(Core::Rectangle(Core::Vector2(topLeft.x(), topLeftCenter.y()), bottomLeftCenter));

    textDesc_->setText(frameWindow_->text());
    Core::Vector2 textSize = textDesc_->textSize();
    Core::Vector2 barSize(textSize.x()+10.0f, topBorderWidth_);

    const float clientWidth = bottomRightCenter.x() - topLeftCenter.x();
    if(barSize.x() > clientWidth) barSize.setX(clientWidth);

    Core::Vector2 barMini = topLeft + Core::Vector2(leftBorderWidth_, 0.0f);

    titleDesc_->draw(Core::Rectangle(Core::Vector2(topLeftCenter.x(), topLeft.y()),
        Core::Vector2(topLeftCenter.x() + barSize.x(), topLeftCenter.y())));

    topDesc_->draw(Core::Rectangle(Core::Vector2(topLeftCenter.x() + barSize.x(), topLeft.y()),
        topRightCenter));


    // ***********************************************************************
    //  draw the title
    // ***********************************************************************

    textDesc_->draw(Core::Rectangle(Core::Vector2(topLeftCenter.x(), topLeft.y()),
                    Core::Vector2(topLeftCenter.x() + barSize.x(), topLeftCenter.y())));

    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// *************************************************************************************************
} // namespace Gooey
