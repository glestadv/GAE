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
#ifndef SCROLLBAR__H
#define SCROLLBAR__H

#include "mmgr/mmgr.h"

#include "glgooey/Slideable.h"
#include "glgooey/Timer.h"
#include "glgooey/RenderDesc.h"


// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ScrollBarSlider;
    class ScrollBarButton;
    class ScrollBarListener;
    class ScrollBarMouseListener;
    class ScrollBarDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A scroll bar with buttons at each end and a slider
class ScrollBar : public Slideable
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the possible justifications of the scroll bar
    enum Justification { horizontal = 0, vertical };



    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief constructs a scroll bar object
    //!
    //! \param aParent the parent window
    //! \param aJustification whether the bar is horizontal or vertical
    //! \param topLeft the topLeft corner of the bar in app window coords
    //! \param extent length of the scroll bar
    ScrollBar(Window* aParent, Justification aJustification, const Core::Vector2& topLeft, float extent);

    //! destructor
    virtual ~ScrollBar();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the justification of the scroll bar (horizontal or vertical)
    Justification justification() const { return justification_; }

    //! \brief returns a value between minimum() and maximum() which indicates what
    //! portion of the range is visible
    int visibleAmount() const { return visibleAmount_; }

    //! \brief sets a value between minimum() and maximum() which indicates what
    //! portion of the range is visible
    ScrollBar& setVisibleAmount(int anAmount)
    {
        visibleAmount_ = anAmount;
        return *this;
    }

    //! returns the color of the track
    Color trackColor() const { return trackColor_; }

    //! sets the track color
    ScrollBar& setTrackColor(const Color& aColor)
    {
        trackColor_ = aColor;
        return *this;
    }

    //! Returns the width of the scroll bar
    float width() const;


    // ---------------------------------------------------------------------------------------------
    //  public interface
    // ---------------------------------------------------------------------------------------------
public:
    // commented in base class
    virtual bool process(const Message& message);
    virtual bool containsFocus() const;
    virtual bool doesHierarchyContain(Window* aWindow) const;

    //! the length of the track
    float trackLength() const;

    //! calculates the length of the slider as a portion of the track length
    float computeSliderLength() const;

    //! \brief The position of the slider according to the scroll bar's value.
    //!
    //! If the slider is being dragged, the actual position will generally
    //! deviate from this value by an offset stored in the slider button
    float discreetSliderOffset() const;

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();


    //! called when the window is resized (rearranges the children)
    virtual void onResize();

    //! called when the window is moved (rearranges the children)
    virtual void onMove();

    //! \brief put all the bits in the right place after the geometry of the bar has
    //! been changed
    ScrollBar& arrange();

    //! slot reacts to the increment button being pressed
    void incrementButtonPressed();

    //! slot reacts to the decrement button being pressed
    void decrementButtonPressed();

    //! slot reacts to the increment button being released
    void incrementButtonReleased();

    //! slot reacts to the decrement button being released
    void decrementButtonReleased();

    //! the scroll bar button for incrementing the value
    ScrollBarButton* incrementButton_;

    //! the scroll bar button for decrementing the value
    ScrollBarButton* decrementButton_;

    //! \brief the timer which allows us to keep moving the bar when the increment button
    //! is held pressed
    Timer incrementTimer_;

    //! \brief the timer which allows us to keep moving the bar when the decrement button
    //! is held pressed
    Timer decrementTimer_;

    //! the slider
    ScrollBarSlider* slider_;

private:
    friend class ScrollBarMouseListener;
    ScrollBarMouseListener* mouseListener_;

    Justification justification_;
    int visibleAmount_;
    Color trackColor_;


    void buttonPressed()  { sliderPressed(); }
    void buttonReleased() { sliderReleased(); }

    ScrollBarDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for scroll bars
class ScrollBarDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ScrollBarDesc();

    //! Destructor
    virtual ~ScrollBarDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const
    {
        if(background_ != 0) background_->draw(area);
        if(border_ != 0)     border_->draw(area);
    }

    //! Returns the width of the scroll bar
    float width() const { return width_; }

    
    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    RenderDesc* background_;
    RenderDesc* border_;
    float width_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(background) &
            TAGGED_OBJECT(border) &
            TAGGED_OBJECT(width);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
