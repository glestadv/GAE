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
#ifndef __GOOEY_REFERENCE_COUNTED_IMAGE_H__
#define __GOOEY_REFERENCE_COUNTED_IMAGE_H__


#include "glgooey/services/interfaces/Image.h"


// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Abstract base class for images that are reference counted.
class ReferenceCountedImage : public Image
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
protected:
    //! Construction requires the name
    ReferenceCountedImage(const std::string& aFileName) : Image(aFileName), referenceCount_(0) {}

    //! Destructor
    virtual ~ReferenceCountedImage() {}



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // commented in base class
    virtual void release()
    {
        decrementReferenceCount();
        if(referenceCount() == 0) delete this;
    }

    //! returns this image's reference count
    size_t referenceCount() const { return referenceCount_; }

    //! Increments this image's reference count
    void incrementReferenceCount() { ++referenceCount_; }

    //! Decrements this image's reference count
    void decrementReferenceCount() { --referenceCount_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    size_t referenceCount_;       //!< The images are reference counted
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Gooey




#endif
