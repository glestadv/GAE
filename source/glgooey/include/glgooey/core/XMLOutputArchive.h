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
#ifndef __GOOEY_CORE_XML_OUTPUT_ARCHIVE_H__
#define __GOOEY_CORE_XML_OUTPUT_ARCHIVE_H__

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>

#include "tinyxml.h"
#include "glgooey/core/Serialization.h"
#include "glgooey/core/Utils.h"



#ifdef _MSC_VER 

// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Archive for saving xml data
class XMLOutputArchive
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Construction requires the stream the archive is to operate on
    XMLOutputArchive(const std::string& aFileName) : document_(aFileName.c_str()), currentNode_(&document_) {};

    //! Destructor
    virtual ~XMLOutputArchive() { document_.SaveFile(); }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! The input/output operator
    template<typename T> XMLOutputArchive& operator& (const ObjectNamer<T>& aNamedObject)
    {
        currentNode_->InsertEndChild(TiXmlElement(aNamedObject.name()));

        TiXmlNode* thisNode = currentNode_;
        currentNode_ = currentNode_->LastChild();
        write(aNamedObject.object(), TypeFinder::typeOf(aNamedObject.object()));
        currentNode_ = thisNode;

        return *this;
    }




    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Write a generic object
    //!
    //! when the compiler works out which \c write method to use, this is the one it falls back
    //! on as a last resort. Any objects that end up being written here must support the
    //! \c serialize method. In general user defined classes will be handled here
    template<typename T> void write(T& anObject, const IsObject&)
    {
        anObject.serialize(*this);
    }


    //! Used to support serialization of object pointers
    template<typename T> void write(T*& anObject, const IsPointer&)
    {
        if(anObject != 0)
        {
            FactoryInterface* factory = FactoryInterface::factoryByClassName(typeid(*anObject).name());
            currentNode_->ToElement()->SetAttribute("className", factory->niceName().c_str());
            factory->serialize(typeid(*this).name(), (void*)(this), (void*&)(anObject));
        }
    }

    //! Used to support serialization of STL vectors
    template<typename T> void write(std::vector<T>& aContainer, const IsObject&) { writeContainer(aContainer); }

    //! Used to support serialization of STL lists
    template<typename T> void write(std::list<T>&     aContainer, const IsObject&) { writeContainer(aContainer); }

    //! Used to support serialization of STL deques
    template<typename T> void write(std::deque<T>&    aContainer, const IsObject&) { writeContainer(aContainer); }

    //! Used to support serialization of STL maps
    template<typename T1, typename T2> void write(std::map<T1, T2>& aContainer, const IsObject&) { writeContainer(aContainer); }

    //! Used to support serialization of std::pair objects as used in maps and multimaps
    template<typename T1, typename T2> void write(std::pair<T1, T2>& aPair, const IsPair&)
    {
        TiXmlNode* thisNode = currentNode_;

        TiXmlElement firstElement("first");
        thisNode->InsertEndChild(firstElement);
        currentNode_ = thisNode->LastChild();
        write(aPair.first);

        TiXmlElement secondElement("second");
        thisNode->InsertEndChild(secondElement);
        currentNode_ = thisNode->LastChild();
        write(aPair.second);

        currentNode_ = thisNode;
    }

    //! Used to support serialization of char values
    void write(char aChar, const IsObject&);

    //! Used to support serialization of unsigned char values
    void write(unsigned char anUnsignedChar, const IsObject&);

    //! Used to support serialization of integer values
    void write(int anInteger, const IsObject&);

    //! Used to support serialization of unsigned integer values
    void write(unsigned int anUnsignedInteger, const IsObject&);

    //! Used to support serialization of boolean values
    void write(bool aBoolean, const IsObject&);

    //! Used to support serialization of floating point values
    void write(float aFloat, const IsObject&);

    //! Used to support serialization of double values
    void write(double aDouble, const IsObject&);

    //! Used to support serialization of constant std::strings
    void write(const std::string& aString, const IsObject&);

    //! Used to support serialization of std::strings
    void write(std::string& aString, const IsObject&);

    //! Generic method for serialization of STL containers
    template<typename Container> void writeContainer(Container& aContainer)
    {
        TiXmlNode* thisNode = currentNode_;
        for(typename Container::iterator it = aContainer.begin(); it != aContainer.end(); ++it)
        {
            TiXmlElement element("item");
            thisNode->InsertEndChild(element);
            currentNode_ = thisNode->LastChild();
            write(*it);
        }
        currentNode_ = thisNode;
    }

    template<typename T> void polymorphicWrite(T*& anObject)
    {
        anObject->serialize(*this);
    }

    TiXmlDocument document_;  //!< The tinyxml document object
    TiXmlNode* currentNode_;  //!< The node that is currently being processed
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey

#else



// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Archive for saving xml data
class XMLOutputArchive
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Construction requires the stream the archive is to operate on
    XMLOutputArchive(const std::string& aFileName) : document_(aFileName.c_str()), currentNode_(&document_) {};

    //! Destructor
    virtual ~XMLOutputArchive() { document_.SaveFile(); }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! The input/output operator
    template<typename T> XMLOutputArchive& operator& (const ObjectNamer<T>& aNamedObject)
    {
        currentNode_->InsertEndChild(TiXmlElement(aNamedObject.name()));

        TiXmlNode* thisNode = currentNode_;
        currentNode_ = currentNode_->LastChild();
        write(aNamedObject.object());
        currentNode_ = thisNode;

        return *this;
    }




    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Write a generic object
    //!
    //! when the compiler works out which \c write method to use, this is the one it falls back
    //! on as a last resort. Any objects that end up being written here must support the
    //! \c serialize method. In general user defined classes will be handled here
    template<typename T> void write(T& anObject)
    {
        anObject.serialize(*this);
    }


    //! Used to support serialization of object pointers
    template<typename T> void write(T*& anObject)
    {
        if(anObject != 0)
        {
            FactoryInterface* factory = FactoryInterface::factoryByClassName(typeid(*anObject).name());
            currentNode_->ToElement()->SetAttribute("className", factory->niceName().c_str());
            factory->serialize(typeid(*this).name(), (void*)(this), (void*&)(anObject));
        }
    }

    //! Used to support serialization of STL vectors
    template<typename T> void write(std::vector<T>&   aContainer) { writeContainer(aContainer); }

    //! Used to support serialization of STL lists
    template<typename T> void write(std::list<T>&     aContainer) { writeContainer(aContainer); }

    //! Used to support serialization of STL deques
    template<typename T> void write(std::deque<T>&    aContainer) { writeContainer(aContainer); }

    //! Used to support serialization of STL maps
    template<typename T1, typename T2> void write(std::map<T1, T2>& aContainer) { writeContainer(aContainer); }

    //! Used to support serialization of std::pair objects as used in maps and multimaps
    template<typename T1, typename T2> void write(std::pair<T1, T2>& aPair)
    {
        TiXmlNode* thisNode = currentNode_;

        TiXmlElement firstElement("first");
        thisNode->InsertEndChild(firstElement);
        currentNode_ = thisNode->LastChild();
        write(aPair.first);

        TiXmlElement secondElement("second");
        thisNode->InsertEndChild(secondElement);
        currentNode_ = thisNode->LastChild();
        write(aPair.second);

        currentNode_ = thisNode;
    }

    //! Used to support serialization of char values
    void write(char aChar);

    //! Used to support serialization of unsigned char values
    void write(unsigned char anUnsignedChar);

    //! Used to support serialization of integer values
    void write(int anInteger);

    //! Used to support serialization of unsigned integer values
    void write(unsigned int anUnsignedInteger);

    //! Used to support serialization of boolean values
    void write(bool aBoolean);

    //! Used to support serialization of floating point values
    void write(float aFloat);

    //! Used to support serialization of double values
    void write(double aDouble);

    //! Used to support serialization of constant std::strings
    void write(const std::string& aString);

    //! Used to support serialization of std::strings
    void write(std::string& aString);

    //! Generic method for serialization of STL containers
    template<typename Container> void writeContainer(Container& aContainer)
    {
        TiXmlNode* thisNode = currentNode_;
        for(typename Container::iterator it = aContainer.begin(); it != aContainer.end(); ++it)
        {
            TiXmlElement element("item");
            thisNode->InsertEndChild(element);
            currentNode_ = thisNode->LastChild();
            write(*it);
        }
        currentNode_ = thisNode;
    }

    template<typename T> void polymorphicWrite(T*& anObject)
    {
        anObject->serialize(*this);
    }

    TiXmlDocument document_;  //!< The tinyxml document object
    TiXmlNode* currentNode_;  //!< The node that is currently being processed
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey

#endif

#endif
