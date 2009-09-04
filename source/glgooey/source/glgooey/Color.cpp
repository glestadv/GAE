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

#include "glgooey/Color.h"

using namespace Gooey;





// ***************************************************************************
//  default constructor
// ***************************************************************************
Color::Color()
{
    setRed  (0);
    setGreen(0);
    setBlue (0);
    setAlpha(1);
}


// ***************************************************************************
//  copy constructor
// ***************************************************************************
Color::Color(const Color& aColor)
{
    (*this) = aColor;
}



// ***************************************************************************
//  assignment operator
// ***************************************************************************
Color& Color::operator = (const Color& aColor)
{
    setRed  (aColor.r()); 
    setGreen(aColor.g()); 
    setBlue (aColor.b()); 
    setAlpha(aColor.a());

    return *this;
}




// ***************************************************************************
//  constructio with separate r, g, b, a values
// ***************************************************************************
Color::Color(float rc, float gc, float bc, float ac)
{
    setRed  (rc); 
    setGreen(gc); 
    setBlue (bc); 
    setAlpha(ac);
}







// ***************************************************************************
//  equality
// ***************************************************************************
bool Color::operator == (const Color& aColor) const
{
    return ( (r() == aColor.r()) &&
             (g() == aColor.g()) &&
             (b() == aColor.b()) &&
             (a() == aColor.a()) );
}
