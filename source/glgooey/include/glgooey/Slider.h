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
#ifndef SLIDER__H
#define SLIDER__H

#include "mmgr/mmgr.h"

#include "glgooey/Slideable.h"
#include "glgooey/RenderDesc.h"

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class SliderButton;
    class SliderMouseListener;
    class SliderDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A fairly typical slider control
class Slider : public Slideable
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the possible justifications of the scroll bar
    enum Justification { horizontal = 0, vertical };

    //! placement of the tick marks
    enum TickPlacement { none = 0, left = 1, below = left, right = 2, above = right, both };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief constructs a slider
    //!
    //! \param aParent the parent window
    //! \param aJustification whether the slider is horizontal or vertical
    //! \param aRectangle the window rectangle of the slider
    Slider(Window* aParent, Justification aJustification, const Core::Rectangle& aRectangle);

    //! destructor
    virtual ~Slider();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the justification of the slider (horizontal or vertical)
    Justification justification() const { return justification_; }

    //! Returns the tick placement of the slider (horizontal or vertical)
    TickPlacement tickPlacement() const { return tickPlacement_; }

    //! Set the tick placement of the slider (horizontal or vertical)
    Slider& setTickPlacement(TickPlacement aTickPlacement)
    { tickPlacement_ = aTickPlacement; return *this; }

    //! \brief Sets the frequency of the tick marks. For example, if this value is set
    //! to 5, there will be a tick mark every five units (not pixels!!) on the slider
    Slider& setTickFrequency(int aFrequency) { tickFrequency_ = aFrequency; return *this; }

    //! \brief Returns the frequency of the tick marks.
    //!
    //! \sa setTickFrequency
    int tickFrequency() const { return tickFrequency_; }


    // ---------------------------------------------------------------------------------------------
    //  public interface
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Make the window attempt to process the passed in message.
    //!
    //! We override this to pass the message on to the contained windows
    virtual bool process(const Message& message);

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! called when the window is resized (rearranges the children)
    virtual void onResize();

    //! called when the window is moved (rearranges the children)
    virtual void onMove();

    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! Returns the length of the track in pixels
    float trackLength() const;

    //! Returns the size of the slider button
    Core::Vector2 buttonSize() const;


private:
    //! \brief put all the bits in the right place after the geometry of the slider has
    //! been changed
    Slider& arrange();

    //! the slider button
    SliderButton* button_;

    Justification justification_;

    TickPlacement tickPlacement_;

    int tickFrequency_;

    SliderMouseListener* mouseListener_;

    SliderDesc* renderDesc_;

    friend class SliderMouseListener;
    friend class SliderButton;

    void buttonPressed()  { sliderPressed(); }
    void buttonReleased() { sliderReleased(); }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc object for the slider
class SliderDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    SliderDesc();

    //! Destructor
    virtual ~SliderDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns the tick color
    Color tickColor() const { return tickColor_; }

    //! Returns the tick width
    float tickWidth() const { return tickWidth_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    RenderDesc* background_;
    RenderDesc* border_;
    Color       tickColor_;
    float       tickWidth_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ---------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(background) &
            TAGGED_OBJECT(border) &
            TAGGED_OBJECT(tickWidth) &
            TAGGED_OBJECT(tickColor);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
