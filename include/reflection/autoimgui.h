#pragma once

#include "reflection.h"

#include <type_traits>
#include <memory>
#include <array>

#include <imgui.h>
#include <magic_enum.hpp>

#include "util.h"

namespace reflection {

class IAutoImGui : public IReflectionBase<IAutoImGui> {
public:
    void DrawAutoImGui();
};

enum class AutoImGuiArg {
    SliderIntMin,
    SliderIntMax,
    SliderFloatMin,
    SliderFloatMax
};

template<>
struct Userdata<IAutoImGui> {
    using Type = std::unordered_map<AutoImGuiArg, std::any>;
};

template<>
class IType<IAutoImGui> {
public:
    virtual void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const = 0;
};

template<>
class Type<IAutoImGui, int> : public TypeBase<IAutoImGui, int> {
public:
    static constexpr ValueType kDefaultVMin = 0;
    static constexpr ValueType kDefaultVMax = 10;

    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto p = static_cast<ValueType*>(addr);
        auto vmin = GetOrDefault(userdata, AutoImGuiArg::SliderIntMin, kDefaultVMin);
        auto vmax = GetOrDefault(userdata, AutoImGuiArg::SliderIntMax, kDefaultVMax);
        ImGui::SliderInt(name, p, vmin, vmax);
    }
};

template<>
class Type<IAutoImGui, bool> : public TypeBase<IAutoImGui, bool> {
public:
    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto p = static_cast<ValueType*>(addr);
        ImGui::Checkbox(name, p);
    }
};

template<>
class Type<IAutoImGui, float> : public TypeBase<IAutoImGui, float> {
public:
    static constexpr ValueType kDefaultVMin = 0.0f;
    static constexpr ValueType kDefaultVMax = 1.0f;

    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto p = static_cast<ValueType*>(addr);
        auto vmin = GetOrDefault(userdata, AutoImGuiArg::SliderFloatMin, kDefaultVMin);
        auto vmax = GetOrDefault(userdata, AutoImGuiArg::SliderFloatMax, kDefaultVMax);
        ImGui::SliderFloat(name, p, vmin, vmax);
    }
};

template<class T>
class Type<IAutoImGui, T, std::enable_if_t<std::is_enum_v<T>>> : public TypeBase<IAutoImGui, T> {
public:
    Type() {
        constexpr auto& names = magic_enum::enum_names<T>();
        auto i = enums.begin();
        auto j = names.begin();
        while (i < enums.end())
            *i++ = *j++;
    }

    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto p = static_cast<T*>(addr);
        char buf[MaxEnumStringViewSize<T>() + 1];
        auto curstrview = magic_enum::enum_name(*p);
        memcpy(buf, &curstrview[0], curstrview.size());
        buf[curstrview.size()] = 0;
        if (ScopeImGuiCombo combo(name, buf); combo) {
            for (const auto& enumstr : enums) {
                bool is_selected = strcmp(buf, enumstr.c_str()) == 0;
                if (ImGui::Selectable(enumstr.c_str(), is_selected))
                    *p = magic_enum::enum_cast<T>(enumstr).value();
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
    }
private:
    std::array<std::string, magic_enum::enum_count<T>()> enums;
};

template<class T>
class _AutoImGuiPointerTypeHelper;

template<class T>
class _AutoImGuiPointerTypeHelper<T*> {
public:
    static void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) {
        auto& p = *static_cast<T**>(addr);

        if (p) {
            ScopeImGuiTreeNode tree(name);
            if (ScopeImGuiPopupContextItem popup; popup) {
                if (ImGui::MenuItem("delete")) {
                    delete p;
                    p = nullptr;
                }
            }
            if (tree && p != nullptr) {
                if constexpr (std::is_base_of_v<IAutoImGui, T>) {
                    for (const auto& [name, fun] : static_cast<IAutoImGui*>(p)->GetFieldTable()) {
                        auto info = fun(p);
                        info.type->DrawAutoImGui(info.address, name.c_str(), info.userdata);
                    }
                }
                else {
                    Type<IAutoImGui, T>::GetIType()->DrawAutoImGui(p, "value", userdata);
                }
            }
        }
        else {
            ImGui::Text("%s is null", name);
            if (ScopeImGuiPopupContextItem popup(name); popup) {
                if constexpr (std::is_base_of_v<IAutoImGui, T> && SubclassInfo<T>::has) {
                    for (const auto& [name, factory] : SubclassInfo<T>::GetFactoryTable()) {
                        if (ImGui::MenuItem(name.c_str())) {
                            p = factory();
                            break;
                        }
                    }
                }
                else {
                    if (ImGui::MenuItem("new")) {
                        p = new T();
                    }
                }
            }
        }
    }
};

template<class _Ty, class _Dx>
class Type<IAutoImGui, std::unique_ptr<_Ty, _Dx>> : public TypeBase<IAutoImGui, std::unique_ptr<_Ty, _Dx>> {
public:
    using typename Type::ValueType;

    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto& v = *static_cast<ValueType*>(addr);
        auto p = v.release();
        _AutoImGuiPointerTypeHelper<_Ty*>::DrawAutoImGui(&p, name, userdata);
        v.reset(p);
    }
};

template<template<class _Ty, class _Alloc> class ContainerType, class _Ty, class _Alloc>
class Type<IAutoImGui, ContainerType<_Ty, _Alloc>
    , std::enable_if_t<std::is_same_v<ContainerType<_Ty, _Alloc>, std::vector<_Ty, _Alloc>>
                    || std::is_same_v<ContainerType<_Ty, _Alloc>, std::list<_Ty, _Alloc>>>>
    : public TypeBase<IAutoImGui, ContainerType<_Ty, _Alloc>> {
public:
    using typename Type::ValueType;

    void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
        auto& v = *static_cast<ValueType*>(addr);

        ScopeImGuiTreeNode tree(name);
        if (ScopeImGuiPopupContextItem popup; popup) {
            if (ImGui::MenuItem("clear")) {
                v.clear();
            }
            else if (ImGui::MenuItem("append")) {
                v.emplace_back();
            }
            else if (ImGui::MenuItem("pop")) {
                if (!v.empty()) {
                    v.pop_back();
                }
            }
        }
        if (tree && !v.empty()) {
            char buf[128];
            unsigned long long i = 0;
            sprintf_s(buf, "%llu", i++);
            Type<IAutoImGui, _Ty>::GetIType()->DrawAutoImGui(&v.front(), buf, userdata);
            for (auto itr = ++v.begin(); itr != v.end(); ++itr) {
                sprintf_s(buf, "exchange(%llu, %llu)", i - 1, i);
                if (ImGui::Button(buf)) {
                    auto pre = itr; --pre;
                    std::swap(*itr, *pre);
                    break;
                }
                sprintf_s(buf, "%llu", i++);
                Type<IAutoImGui, _Ty>::GetIType()->DrawAutoImGui(&*itr, buf, userdata);
            }
        }
    }
};

template<class T>
struct _AutoImGuiMapTypeHelper {
    using _Ty = typename T::mapped_type;

    static void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) {
        auto& v = *static_cast<T*>(addr);

        ScopeImGuiTreeNode tree(name);
        if (ScopeImGuiPopupContextItem popup; popup) {
            static char keybuf[128];
            if (ImGui::MenuItem("clear")) {
                v.clear();
            }
            ImGui::InputText("##keyinput", keybuf, IM_ARRAYSIZE(keybuf));
            ImGui::SameLine();
            if (ImGui::Button("add")) {
                std::pair<const char*, _Ty> pair;
                pair.first = keybuf;
                v.insert(std::move(pair));
            }
            
        }
        if (tree) {
            char buf[128];
            unsigned long long i = 0;
            for (auto itr = v.begin(); itr != v.end(); ++itr) {
                auto& [key, value] = *itr;
                Type<IAutoImGui, _Ty>::GetIType()->DrawAutoImGui(&value, key.c_str(), userdata);
                sprintf_s(buf, "erase##%llu", i++);
                if (ImGui::Button(buf)) {
                    v.erase(itr);
                    break;
                }
            }
        }
    }
};

template<template<class _Kty, class _Ty, class _Pr, class _Alloc> class ContainerType,
    class _Kty, class _Ty, class _Pr, class _Alloc>
    class Type<IAutoImGui, ContainerType<_Kty, _Ty, _Pr, _Alloc>, std::enable_if_t<std::is_same_v<std::string, _Kty>>>
    : public TypeBase<IAutoImGui, ContainerType<_Kty, _Ty, _Pr, _Alloc>> {
    public:
        using typename Type::ValueType;

        void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
            _AutoImGuiMapTypeHelper<ValueType>::DrawAutoImGui(addr, name, userdata);
        }
};

template<template <class _Kty, class _Ty, class _Hasher, class _Keyeq, class _Alloc> class ContainerType,
    class _Kty, class _Ty, class _Hasher, class _Keyeq, class _Alloc>
    class Type<IAutoImGui, ContainerType<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>, std::enable_if_t<std::is_same_v<std::string, _Kty>>>
    : public TypeBase<IAutoImGui, ContainerType<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>> {
    public:
        using typename Type::ValueType;

        void DrawAutoImGui(void* addr, const char* name, const Userdata<IAutoImGui>::Type& userdata) const override {
            _AutoImGuiMapTypeHelper<ValueType>::DrawAutoImGui(addr, name, userdata);
        }
};

inline void IAutoImGui::DrawAutoImGui() {
    for (const auto& [name, fun] : GetFieldTable()) {
        auto info = fun(this);
        info.type->DrawAutoImGui(info.address, name.c_str(), info.userdata);
    }
}

} // namespace reflection
