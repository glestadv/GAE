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
#ifndef INTERVALWINDOW__H
#define INTERVALWINDOW__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Abstract base class for all windows that represent an integer interval and a value
//! within that interval, like scroll bars or sliders.
class Slideable : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor requires the parent window
    Slideable(Window* aParent);

    //! destructor
    virtual ~Slideable();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the value that the slideable represents
    int value() const { return value_; }

    //! sets the value that the slideable represents
    Slideable& setValue(int aValue);

    //! returns the maximum value of the slideable
    int maximum() const { return maximum_; }

    //! sets the maximum value of the slideable
    Slideable& setMaximum(int aValue);

    //! returns the minimum value of the slideable
    int minimum() const { return minimum_; }

    //! sets the minimum value of the slideable
    Slideable& setMinimum(int aValue);

    //! \brief values can be increased per unit (for instance by clicking
    //! on the arrow buttons of a scroll bar). This returns the actual
    //! increment in such a case
    int unitIncrement() const { return unitIncrement_; }

    //! \brief values can be increased per unit (for instance by clicking
    //! on the arrow buttons of a scroll bar). This sets the actual
    //! increment in such a case
    Slideable& setUnitIncrement(int aValue)
    {
        unitIncrement_ = aValue;
        return *this;
    }

    //! \brief values can be increased per block (for instance by clicking
    //! on the track of a scroll bar). This returns the actual
    //! increment in such a case
    int blockIncrement() const { return blockIncrement_; }

    //! \brief values can be increased per block (for instance by clicking
    //! on the track of a scroll bar). This sets the actual
    //! increment in such a case
    Slideable& setBlockIncrement(int aValue)
    {
        blockIncrement_ = aValue;
        return *this;
    }

    //! \brief Returns the range of the slideable (i.e. maximum() - minimum()).
    //! If maximum is less than minimum an exception is thrown.
    unsigned int range() const;


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! Emitted when the slideable's value changes
    Signal1<int> valueChanged;

    //! Emitted when the slideable's button is pressed
    Signal0 sliderPressed;

    //! Emitted when the slideable's button is released
    Signal0 sliderReleased;

    //! Emitted when the slideable's button is moved. The current slider value is passed on.
    Signal1<int> sliderMoved;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    int value_;
    int maximum_;
    int minimum_;
    int unitIncrement_;
    int blockIncrement_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
