#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <any>

// Declare in class declaration
#define FIELD_DECLARATION_BEGIN(classname, interfacename)                         \
    const reflection::IReflectionBase<interfacename>::FieldTable& GetFieldTable(  \
            interfacename* = nullptr) const override {                            \
        using Class = classname;                                                  \
        using InterfaceType = interfacename;                                      \
        static const reflection::IReflectionBase<InterfaceType>::FieldTable m{

#define FIELD_DECLARATION(name, field, ...)                                       \
    { name, [](InterfaceType* arg) {                                              \
        static reflection::Userdata<InterfaceType>::Type userdata __VA_ARGS__;    \
        auto p = static_cast<Class*>(arg);                                        \
        return reflection::IReflectionBase<InterfaceType>::FieldInfo {            \
            static_cast<void*>(&(p->field)),                                      \
            reflection::Type<InterfaceType, decltype(p->field)>::GetIType(),      \
            userdata }; }},

#define FIELD_DECLARATION_END() }; return m; }


// Declare out of class declaration
// Subclass header should be included
#define SUBCLASS_DECLARATION_BEGIN(classname)                    \
    template<>                                                   \
    struct reflection::SubclassInfo<classname> {                 \
        using Class = classname;                                 \
        using FactoryFunc = Class * (*) ();                      \
        using FactoryTable = std::map<std::string, FactoryFunc>; \
        static constexpr bool has = true;                        \
        static const FactoryTable& GetFactoryTable() {           \
            static const FactoryTable m {

#define SUBCLASS_DECLARATION(subclass)                   \
    { typeid(subclass).name(), []()                      \
        { return static_cast<Class*>(new subclass()); }},

#define SUBCLASS_DECLARATION_END()  }; return m; } };


namespace reflection {

template<class I>
class IType;

template <class I>
struct Userdata {
    // Default userdata type
    using Type = std::unordered_map<int, std::any>;
};

template <class I>
class IReflectionBase {
public:
    struct FieldInfo {
        void* address;
        const IType<I>* type;
        const typename Userdata<I>::Type& userdata;
    };

    using GetFieldFunc = FieldInfo(*) (I*);
    using FieldTable = std::map<std::string, GetFieldFunc>;

    // Arg is used to avoid signature collision when a class implements multiple interfaces
    virtual const FieldTable& GetFieldTable(I* = nullptr) const = 0;

    virtual ~IReflectionBase() {};
};

template<class I, class T, class Enable = void>
class Type;

template<class I, class T>
class TypeBase : public IType<I> {
public:
    using ValueType = T;

    static const IType<I>* GetIType() {
        static const Type<I, T> instance;
        return &instance;
    }
protected:
    TypeBase() {};
};


template<class T>
struct SubclassInfo {
    static constexpr bool has = false;
};

} // namespace reflection
