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

#include "glgooey/Slideable.h"

#include "glgooey/core/StandardException.h"
#include "glgooey/SlideableMessage.h"


using namespace Gooey;


// ***************************************************************************
//  default constructor
// ***************************************************************************
Slideable::Slideable(Window* aParent) :
    Window(aParent, ""),
    maximum_(255),
    minimum_(0)
{
    setValue(0);
    setUnitIncrement(1);
    setBlockIncrement(64);
}




// ***************************************************************************
//  destructor
// ***************************************************************************
Slideable::~Slideable()
{
}




// ***************************************************************************
//  sets the value that the slideable represents
// ***************************************************************************
Slideable&
Slideable::setValue(int aValue)
{ 
    value_ = aValue; 

    if(value() < minimum()) 
        setValue(minimum());
    else if(value() > maximum())
        setValue(maximum());

    process(SlideableMessage(value()));
    valueChanged(value());

    return *this; 
}





// ***************************************************************************
//   Returns the range of the slideable (i.e. maximum() - minimum()).
//  If maximum is less than minimum an exception is thrown.
// ***************************************************************************
unsigned int
Slideable::range() const
{
    const int mini = minimum();
    const int maxi = maximum();
    if(maxi < mini) THROW("Slideable range is invalid");
    return (maxi - mini);
}




// ***************************************************************************
Slideable&
Slideable::setMaximum(int aValue)
{
    ASSERT(aValue > minimum());
    maximum_ = aValue;
    if(maximum() < value()) setValue(maximum());
    return *this;
}
// ***************************************************************************




// ***************************************************************************
Slideable&
Slideable::setMinimum(int aValue)
{
    ASSERT(aValue < maximum());
    minimum_ = aValue;
    if(minimum() > value()) setValue(minimum());
    return *this;
}
// ***************************************************************************

