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
#ifndef __GOOEY_DEVIL_IMAGE_H__
#define __GOOEY_DEVIL_IMAGE_H__


#include "glgooey/services/interfaces/ReferenceCountedImage.h"

#include "glgooey/core/Position.h"

class ilImage;

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    namespace Core
    {
        class Size;
    }
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief image loaded using DevIL - This is an example of an image service
class DevILImage : public ReferenceCountedImage
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
protected:
    //! Construction requires the name
    DevILImage(const std::string& aFileName);

    //! Destructor
    virtual ~DevILImage();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in the base class
    virtual Core::Size size() const;
    virtual size_t numberOfBitsPerPixel() const;
    virtual const unsigned char* data() const;
    virtual unsigned char* data();



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    ilImage* image_;   //!< A pointer to the ilImage object wrapped by this class

    friend class Image;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// *************************************************************************************************
} // namespace Gooey




#endif
