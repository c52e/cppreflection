#pragma once

#include "reflection.h"

#include <type_traits>
#include <memory>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <magic_enum.hpp>

#include "util.h"

namespace reflection {

class ISerialization : public IReflectionBase<ISerialization> {
public:
    void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);

    void Deserialize(const rapidjson::Value&);
};

template<>
class IType<ISerialization> {
public:
    virtual void Serialize(const void*, rapidjson::PrettyWriter<rapidjson::StringBuffer>&) const = 0;

    virtual void Deserialize(void*, const rapidjson::Value&) const = 0;
};

template<>
class Type<ISerialization, int> : public TypeBase<ISerialization, int> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.Int(v);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsInt());
        auto& v = *static_cast<ValueType*>(addr);
        v = value.GetInt();
    }
};

template<>
class Type<ISerialization, bool> : public TypeBase<ISerialization, bool> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.Bool(v);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsBool());
        auto& v = *static_cast<ValueType*>(addr);
        v = value.GetBool();
    }
};

template<>
class Type<ISerialization, float> : public TypeBase<ISerialization, float> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.Double(static_cast<double>(v));
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsNumber());
        auto& v = *static_cast<ValueType*>(addr);
        v = value.GetFloat();
    }
};

template<>
class Type<ISerialization, double> : public TypeBase<ISerialization, double> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.Double(v);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsNumber());
        auto& v = *static_cast<ValueType*>(addr);
        v = value.GetDouble();
    }
};

template<>
class Type<ISerialization, std::string> : public TypeBase<ISerialization, std::string> {
public:
    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.String(v.c_str());
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsString());
        auto& v = *static_cast<ValueType*>(addr);
        v = value.GetString();
    }
};

template<class T>
class Type<ISerialization, T, std::enable_if_t<std::is_enum_v<T>>> : public TypeBase<ISerialization, T> {
public:
    static_assert(!std::is_same_v<ISerialization, T>);

    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto v = *static_cast<const T*>(addr);
        writer.String(std::string(magic_enum::enum_name(v)).c_str());
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsString());
        auto e = magic_enum::enum_cast<T>(value.GetString());
        R_ASSERT(e.has_value());
        auto& v = *static_cast<T*>(addr);
        v = e.value();
    }
};

template<class T, class Enable = void>
class _SerializationPointerTypeHelper;

template<class T>
class _SerializationPointerTypeHelper<T*, std::enable_if_t<std::is_base_of_v<ISerialization, T> && !SubclassInfo<T>::has>> {
public:
    static void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
        const auto v = *static_cast<T*const*>(addr); // Must NOT directly convert from void* to baseclass*

        if (v) {
            writer.StartObject();
            for (const auto& [name, fun] : static_cast<ISerialization*>(v)->GetFieldTable()) {
                writer.String(name.c_str());
                auto info = fun(v);
                info.type->Serialize(info.address, writer);
            }
            writer.EndObject();
        }
        else {
            writer.Null();
        }
    }

    static void Deserialize(void* addr, const rapidjson::Value& value) {
        auto& v = *static_cast<T**>(addr);
        if (value.IsNull()) {
            delete v;
            v = nullptr;
        }
        else {
            R_ASSERT(value.IsObject());
            if (v == nullptr)
                v = new T();
            for (const auto& [name, fun] : v->GetFieldTable(static_cast<ISerialization*>(nullptr))) {
                auto itr = value.FindMember(name.c_str());
                R_ASSERT(itr != value.MemberEnd());
                auto info = fun(v);
                info.type->Deserialize(info.address, itr->value);
            }
        }
    }
};

template<class T>
class _SerializationPointerTypeHelper<T*, std::enable_if_t<std::is_base_of_v<ISerialization, T>&& SubclassInfo<T>::has>> {
public:
    static constexpr auto kTypeKey = "type";
    static constexpr auto kDataKey = "data";

    static void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
        const auto v = *static_cast<T* const*>(addr);
        if (v) {
            writer.StartObject();
            writer.String(kTypeKey);
            writer.String(typeid(*v).name());
            writer.String(kDataKey);
            v->Serialize(writer);
            writer.EndObject();
        }
        else {
            writer.Null();
        }
    }

    static void Deserialize(void* addr, const rapidjson::Value& value) {
        R_ASSERT(value.IsObject() || value.IsNull());
        auto& v = *static_cast<T**>(addr);
        delete v; // Always delete polymorphic pointer
        v = nullptr;
        if (value.IsObject()) {
            auto typeitr = value.FindMember(kTypeKey);
            R_ASSERT(typeitr != value.MemberEnd());
            auto& name = typeitr->value;
            R_ASSERT(name.IsString());
            v = SubclassInfo<T>::GetFactoryTable().at(name.GetString())();

            auto dataitr = value.FindMember(kDataKey);
            R_ASSERT(dataitr != value.MemberEnd());
            v->Deserialize(dataitr->value);
        }
    }
};

template<class T>
class _SerializationPointerTypeHelper<T*, std::enable_if_t<!std::is_base_of_v<ISerialization, T>>> {
public:
    static void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
        const auto v = *static_cast<const T* const*>(addr);
        if (v)
            Type<ISerialization, T>::GetIType()->Serialize(v, writer);
        else
            writer.Null();
    }

    static void Deserialize(void* addr, const rapidjson::Value& value) {
        auto& v = *static_cast<T**>(addr);
        if (value.IsNull()) {
            delete v;
            v = nullptr;
        }
        else {
            if (v == nullptr)
                v = new T();
            Type<ISerialization, T>::GetIType()->Deserialize(v, value);
        }
    }
};

template<class _Ty, class _Dx>
class Type<ISerialization, std::unique_ptr<_Ty, _Dx>> : public TypeBase<ISerialization, std::unique_ptr<_Ty, _Dx>> {
public:
    using typename Type::ValueType;

    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        auto p = v.get();
        _SerializationPointerTypeHelper<_Ty*>::Serialize(&p, writer);
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        auto& v = *static_cast<ValueType*>(addr);
        _Ty* tmp = nullptr;
        _SerializationPointerTypeHelper<_Ty*>::Deserialize(&tmp, value);
        v.reset(tmp);
    }
};

template<template<class _Ty, class _Alloc> class ContainerType, class _Ty, class _Alloc>
class Type<ISerialization, ContainerType<_Ty, _Alloc>
    , std::enable_if_t<std::is_same_v<ContainerType<_Ty, _Alloc>, std::vector<_Ty, _Alloc>>
                    || std::is_same_v<ContainerType<_Ty, _Alloc>, std::list<_Ty, _Alloc>>>>
    : public TypeBase<ISerialization, ContainerType<_Ty, _Alloc>> {
public:
    using typename Type::ValueType;

    void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
        const auto& v = *static_cast<const ValueType*>(addr);
        writer.StartArray();
        for (const auto& e : v) {
            Type<ISerialization, _Ty>::GetIType()->Serialize(&e, writer);
        }
        writer.EndArray();
    }

    void Deserialize(void* addr, const rapidjson::Value& value) const override {
        R_ASSERT(value.IsArray());
        auto& v = *static_cast<ValueType*>(addr);

        v.clear();
        for (const auto& e : value.GetArray()) {
            _Ty tmp{};
            Type<ISerialization, _Ty>::GetIType()->Deserialize(&tmp, e);
            v.emplace_back(std::move(tmp));
        }
    }
};

template<class T>
struct _SerializationMapTypeHelper {
    using _Ty = typename T::mapped_type;

    static void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
        const auto& v = *static_cast<const T*>(addr);
        writer.StartObject();
        for (const auto& [key, value] : v) {
            writer.String(key.c_str());
            Type<ISerialization, _Ty>::GetIType()->Serialize(&value, writer);
        }
        writer.EndObject();
    }

    static void Deserialize(void* addr, const rapidjson::Value& value) {
        R_ASSERT(value.IsObject());
        auto& v = *static_cast<T*>(addr);
        
        v.clear();
        for (const auto& e : value.GetObject()) {
            _Ty tmp{};
            Type<ISerialization, _Ty>::GetIType()->Deserialize(&tmp, e.value);
            v.emplace(e.name.GetString(), std::move(tmp));
        }
    }
};

template<template<class _Kty, class _Ty, class _Pr, class _Alloc> class ContainerType,
    class _Kty, class _Ty, class _Pr, class _Alloc>
    class Type<ISerialization, ContainerType<_Kty, _Ty, _Pr, _Alloc>, std::enable_if_t<std::is_same_v<std::string, _Kty>>>
    : public TypeBase<ISerialization, ContainerType<_Kty, _Ty, _Pr, _Alloc>> {
    public:
        using typename Type::ValueType;

        void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
            _SerializationMapTypeHelper<ValueType>::Serialize(addr, writer);
        }

        void Deserialize(void* addr, const rapidjson::Value& value) const override {
            _SerializationMapTypeHelper<ValueType>::Deserialize(addr, value);
        }
};

template<template <class _Kty, class _Ty, class _Hasher, class _Keyeq, class _Alloc> class ContainerType,
    class _Kty, class _Ty, class _Hasher, class _Keyeq, class _Alloc>
    class Type<ISerialization, ContainerType<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>, std::enable_if_t<std::is_same_v<std::string, _Kty>>>
    : public TypeBase<ISerialization, ContainerType<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>> {
    public:
        using typename Type::ValueType;

        void Serialize(const void* addr, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override {
            _SerializationMapTypeHelper<ValueType>::Serialize(addr, writer);
        }

        void Deserialize(void* addr, const rapidjson::Value& value) const override {
            _SerializationMapTypeHelper<ValueType>::Deserialize(addr, value);
        }
};

inline void ISerialization::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
    writer.StartObject();
    for (const auto& [name, fun] : GetFieldTable()) {
        writer.String(name.c_str());
        auto info = fun(this);
        info.type->Serialize(info.address, writer);
    }
    writer.EndObject();
}

inline void ISerialization::Deserialize(const rapidjson::Value& value) {
    R_ASSERT(value.IsObject());
    for (const auto& [name, fun] : GetFieldTable()) {
        auto itr = value.FindMember(name.c_str());
        R_ASSERT(itr != value.MemberEnd());
        auto info = fun(this);
        info.type->Deserialize(info.address, itr->value);
    }
}

} // namespace reflection
