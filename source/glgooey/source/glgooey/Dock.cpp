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

#include "mmgr/mmgr.h"

#include "glgooey/Dock.h"



// *************************************************************************************************
namespace Gooey
{


void
Dock::computePosition(Core::Vector2& position,const Core::Vector2& size,const Core::Vector2& parentSize) const
{
    if (!docked())
        return;

    if (top_)
        position.setY(margin_.y());
    else
        position.setY(parentSize.y() - size.y() - margin_.y());

    if (left_)
        position.setX(margin_.x());
    else
        position.setX(parentSize.x() - size.x() - margin_.x());
}


// *************************************************************************************************
} // namespace Gooey
