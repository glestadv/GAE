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

#include "devILImage/DevILImage.h"

#include "glgooey/core/StandardException.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/il_wrap.h>

// *************************************************************************************************
namespace Gooey
{




// *************************************************************************************************
namespace
{

std::map<std::string, DevILImage*> imageMap;

// *************************************************************************************************
} // anonymous namespace





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Image*
Image::create(const std::string& aFileName)
{
    static bool isDevILInitialized = false;
    if(!isDevILInitialized)
    {
        glPushAttrib(GL_ENABLE_BIT);

        ilInit();
        iluInit();
        ilutRenderer(ILUT_OPENGL);
        isDevILInitialized = true;

        glPopAttrib();
    }

    std::map<std::string, DevILImage*>::iterator imageIterator = imageMap.find(aFileName);
    if(imageIterator == imageMap.end())
    {
        imageMap[aFileName] = new DevILImage(aFileName);
    }

    imageIterator = imageMap.find(aFileName);
    ASSERT(imageIterator != imageMap.end());

    imageIterator->second->incrementReferenceCount();

    return imageIterator->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DevILImage::DevILImage(const std::string& aFileName) :
    ReferenceCountedImage(aFileName)
{
    image_ = new ilImage(const_cast<char*>(aFileName.c_str()));

    image_->Flip();

    if(image_->Format() != IL_RGBA) image_->Convert(IL_RGBA);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DevILImage::~DevILImage()
{
    ASSERT(referenceCount() == 0);

    imageMap.erase(imageMap.find(fileName()));

    delete image_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Size
DevILImage::size() const
{
    return Core::Size(image_->Width(), image_->Height());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
DevILImage::numberOfBitsPerPixel() const
{
    return image_->Bitpp();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const unsigned char*
DevILImage::data() const
{
    return image_->GetData();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned char*
DevILImage::data()
{
    return image_->GetData();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// *************************************************************************************************
} // namespace Gooey

