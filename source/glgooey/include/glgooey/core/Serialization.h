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
#ifndef __GOOEY_CORE_SERIALIZATION_H__
#define __GOOEY_CORE_SERIALIZATION_H__


#include <utility>

#include "glgooey/core/StandardException.h"

// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{



#ifdef _MSC_VER 

struct IsPointer {};
struct IsPair {};
struct IsObject {};

struct TypeFinder
{
    static IsPointer typeOf(const void*) { return IsPointer(); }
    template<typename K, typename V> static IsPair typeOf(const std::pair<K, V>&) { return IsPair(); }
    static IsObject  typeOf(...) { return IsObject(); }
};

#endif



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Base class and interface to objects used to serialize pointers
//!
//! Because our serialization is template based, there is a problem with polymorphism that must
//! be overcome. The problem is that at compile time when the compiler needs to instantiate the
//! serialization code, it cannot be aware of the classes that need to be serialized because
//! the type of pointer used to do the serialization is a pointer to the base class. But in
//! order for the serialization of derived types to work, the compiler must instantiate code
//! for each derived type and each archive. On the other hand, the code which is linked at run time
//! cannot be template based. We solve this problem by exploiting a class factory with which all
//! polymorphic classes must be registered. Each registration is followed by the instantiation of
//! one Serializer object per archive type that
//!  - forces the instantiation of the code that serializes the registered type for that archive type
//!  - derives from SerializerInterface which provides a template-free serialize method to which
//!       the serialization code can connect at run time.
class SerializerInterface
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Destructor
    virtual ~SerializerInterface() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the name of the archive type as given by RTTI
    virtual std::string archiveName() = 0;

    //! \brief Because derived classes know the true types of archive and object, they can cast
    //! appropriately and execute the correct code
    virtual void serialize(void* anArchive, void*& anObject) = 0;

    //! \brief Returns the next serializer in the factory's linked list
    //! \see nextSerializer_
    SerializerInterface* nextSerializer() const { return nextSerializer_; }

    //! \brief Sets the next serializer in the factory's linked list
    //! \see nextSerializer_
    void setNextSerializer(SerializerInterface* aSerializer) { nextSerializer_ = aSerializer; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    //! Serializers are stored in a linked list on a per factory basis
    SerializerInterface* nextSerializer_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Base class and interface to class factory
class FactoryInterface
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Destructor
    virtual ~FactoryInterface() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Creates an instance of the class with the passed in name
    static void* createClass(const std::string& aName)
    {
        return factoryByNiceName(aName)->create();
    }

    //! Returns the name of the class (as given by type_info) which can be created by this factory
    virtual std::string className() const = 0;

    //! Returns a name given to the class which can be created by this factory
    virtual std::string niceName() const = 0;

    //! Returns a pointer to the next factory in the linked list
    FactoryInterface* nextFactory() { return nextFactory_; }

    //! Returns the factory for the class type with the passed in name as given by RTTI
    static FactoryInterface* factoryByClassName(const std::string& aName);

    //! \brief Returns the factory for the class type with the passed in name as specified in the
    //! factory's constructor
    static FactoryInterface* factoryByNiceName(const std::string& aName);

    //! Returns a pointer to the next factory in the linked list
    void setNextFactory(FactoryInterface* aFactory) { nextFactory_ = aFactory; }

    //! Derived classes override this method to do the actual creation
    virtual void* create() const = 0;

    //! \brief Serializes the passed in object to the passed in archive whose type is specified
    //! via the passed in archive class name
    virtual void serialize(const std::string& anArchiveClassName, void* anArchive, void*& anObject) = 0;

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    static FactoryInterface* firstFactory_; //!< Head of the linked list of factory objects
    FactoryInterface* nextFactory_; //!< Pointer to the next item in the linked list of factories
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Template based class factory
//!
//! In order to support serialization of polymorphic types each polymorphic type must be registered
//! with the class factory this is done via a simple declaration:
//! \code
//!   Factory<MyClassType> MyFactory("MyClassType");
//! \endcode
//! It is however recommended that the REGISTER_CLASS macro is used, as this also automatically
//! adds the support for the various archive types.
template<typename T>
class Factory : public FactoryInterface
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Constructor
    Factory(const std::string& aName) : className_(typeid(T).name()), niceName_(aName), firstSerializer_(0)
    {
        nextFactory_ = firstFactory_;
        firstFactory_ = this;
    }

    //! Destructor
    virtual ~Factory()
    {
        FactoryInterface* prevFactory = 0;
        FactoryInterface* factory = firstFactory_;
        while(factory != 0 && factory != this)
        {
            prevFactory = factory;
            factory = factory->nextFactory();
        }
        if( (factory != 0) && (prevFactory != 0) ) prevFactory->setNextFactory(factory->nextFactory());
    }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in the base class
    virtual std::string className() const { return className_; }
    virtual std::string niceName() const { return niceName_; }
    virtual void* create() const { return reinterpret_cast<void*>(new T); }
    virtual void serialize(const std::string& anArchiveClassName, void* anArchive, void*& anObject)
    {
        findSerializerByArchiveName(anArchiveClassName)->serialize(anArchive, anObject);
    }

    //! Adds the passed in serializer to this factory's list of serializers
    void registerSerializer(SerializerInterface* aSerializer)
    {
        aSerializer->setNextSerializer(firstSerializer_);
        firstSerializer_ = aSerializer;
    }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Returns the serializer that matches the passed in archive name and the type of
    //! object this factory is responsible for
    SerializerInterface* findSerializerByArchiveName(const std::string& anArchiveClassName)
    {
        SerializerInterface* serializer = firstSerializer_;
        while(serializer->archiveName() != anArchiveClassName)
        {
            serializer = serializer->nextSerializer();
            if(serializer == 0) throw "trying to access unregistered serializer type";
        }
        return serializer;
    }

    std::string className_; //!< The name of the class which can be created by this factory
    std::string niceName_;  //!< The name given to the class on construction of the factory object

    SerializerInterface* firstSerializer_; //!< The first serializer in the factory's linked list
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Serializer for a particular archive and a particular polymorphic type
template<typename Archive, typename T>
class Serializer : public SerializerInterface
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief The constructor requires the factory for the polymorphic type specified in the
    //! template parameters
    Serializer(Factory<T>& aFactory) { aFactory.registerSerializer(this); }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in base class
    virtual std::string archiveName() { return typeid(Archive).name(); }
    virtual void serialize(void* anArchive, void*& anObject)
    {
        Archive* archive = reinterpret_cast<Archive*>(anArchive);
        T*& object = reinterpret_cast<T*&>(anObject);
        object->serialize(*archive);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




#define REGISTER_CLASS(x) \
Gooey::Core::Factory<x> __##x##Factory__(#x); \
Gooey::Core::Serializer<Gooey::Core::XMLOutputArchive, x> __##x##XMLOutputSerializer__(__##x##Factory__); \
Gooey::Core::Serializer<Gooey::Core::XMLInputArchive, x> __##x##XMLInputSerializer__(__##x##Factory__)




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Class used to give names to objects
template<typename T>
class ObjectNamer
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Construction requires a reference to the object and the name to give the object
    ObjectNamer(const char* aName, T& anObject) : pair_(aName, &anObject) {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the name of the object
    const char* name() const { return pair_.first; }

    //! Returns a reference to the object being named
    T& object() const { return *pair_.second; }

    //! reads/writes the object from/to the passed in archive
    template<typename Archive> void serialize(Archive& anArchive) { anArchive & object(); }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    std::pair<const char*, T*> pair_;  //!< The pair which maps a name to an object
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename T>
ObjectNamer<T> nameObject(T& anObject, const char* aName)
{
    return ObjectNamer<T>(aName, anObject);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define TAGGED_OBJECT(x) Gooey::Core::nameObject(x##_, #x)



// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey



#endif
