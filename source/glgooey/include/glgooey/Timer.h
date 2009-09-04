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
#ifndef TIMER__H
#define TIMER__H

#include "mmgr/mmgr.h"

#include "sigslot.h"


// *************************************************************************************************
namespace Gooey
{

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A millisecond accurate timer
//!
//! Derive from this class to create a timer which is called from the
//! WindowManager's update method whenever its interval has elapsed. For
//! an example of how to do this see the ScrollBarButtonTimer class in the
//! file ScrollBar.h
class Timer
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor - default interval of one second
    explicit Timer(float anInterval = 1.0);

    //! destructor
    virtual ~Timer();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the timer's interval in seconds
    float interval() const { return interval_; }

    //! Sets the timer's interval in seconds
    Timer& setInterval(float seconds) { interval_ = seconds; return *this; }

    //! returns whether the timer is currently running or not
    bool isStarted() const { return isStarted_; }

    //! has the timer elapsed?
    bool hasElapsed();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! start the timer
    Timer& start();

    //! stop the timer
    Timer& stop();


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! This signal is emitted when the timer times out
    Signal0 trigger;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    float interval_;        //!< The interval between each trigger signal
    bool isStarted_;        //!< Whether the timer is currently running or not
    float lastTriggerTime_; //!< the time when the timer last triggered an message
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
