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
#ifndef PROGRESSBAR__H
#define PROGRESSBAR__H

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
    class ProgressBarDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A simple progress bar.
class ProgressBar : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! Progress bars can be either horizontally or vertically orientated
    enum Orientation
    {
        horizontal = 0, //!< the bar progresses from left to right
        vertical        //!< the bar progresses from bottom to top
    };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Constructs a progress bar
    //!
    //! Constructor requires the parent window and the rectangle which is to
    //! be filled with the progress bar. The orientation may also be
    //! specified. If no orientation is specified, the bar will be horizontal
    //! an indicate progress from left to right
    ProgressBar(Window* aParent, const Core::Rectangle& aRectangle,
        const Orientation anOrientation = horizontal);

    //! destructor
    virtual ~ProgressBar();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the orientation of the bar
    Orientation orientation() const { return orientation_; }

    //! sets the orientation of the bar
    ProgressBar& setOrientation(Orientation o)
    {
        orientation_ = o;
        return *this;
    }

    //! returns the percentage value that the progress bar is to represent
    float percentage() const { return percentage_; }

    //! sets the percentage value that the progress bar is to represent
    ProgressBar& setPercentage(float p)
    {
        percentage_ = p;
        return *this;
    }



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

private:
    Orientation orientation_;
    float percentage_;

    ProgressBarDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for progress bars
class ProgressBarDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ProgressBarDesc();

    //! Destructor
    virtual ~ProgressBarDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! sets the button object that will be drawn on the next call to draw
    ProgressBarDesc& setProgressBar(ProgressBar* aProgressBar)
    {
        progressBar_ = aProgressBar;
        return *this;
    }


private:
    RenderDesc*  horzEmptyDesc_;
    RenderDesc*  horzFullDesc_;
    RenderDesc*  vertEmptyDesc_;
    RenderDesc*  vertFullDesc_;
    RenderDesc*  border_;
    ProgressBar* progressBar_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(horzEmptyDesc) &
            TAGGED_OBJECT(horzFullDesc) &
            TAGGED_OBJECT(vertEmptyDesc) &
            TAGGED_OBJECT(vertFullDesc) &
            TAGGED_OBJECT(border);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
