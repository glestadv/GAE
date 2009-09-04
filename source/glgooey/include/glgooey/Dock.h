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
//
//  Author(s):
//    Sebastien Berthet (sbrt@yahoo.fr)
//
// __________________________________________________________________________________________________
#ifndef DOCK__H
#define DOCK__H

#include "mmgr/mmgr.h"

#include "glgooey/core/Vector2.h"



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class Dock
{
public:
    Dock() : docked_(false) {}
    virtual ~Dock() {}

    void setDocked(bool d) { docked_=d; }
    void setTop(bool t)  { top_=t;  }
    void setLeft(bool l) { left_=l; }
    void setMargin(const Core::Vector2& m) { margin_=m; }

    bool docked() const { return docked_; }
    bool top()  const { return top_;  }
    bool left() const { return left_; }
    const Core::Vector2& margin() const { return margin_; }

    void dock(bool t,bool l,const Core::Vector2& m)
    { setDocked(true); setTop(t); setLeft(l); setMargin(m);    }
    void undock()
    { setDocked(false); }

    void computePosition(Core::Vector2& position,const Core::Vector2& size,const Core::Vector2& parentSize) const;

private:
    bool        docked_;
    bool        top_;
    bool        left_;
    Core::Vector2        margin_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace

#endif
