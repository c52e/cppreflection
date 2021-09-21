#pragma once

#include <stdexcept>
#include <unordered_map>
#include <any>

#include <magic_enum.hpp>
#include <imgui.h>


#define R_ASSERT(exp)                                                               \
    if (!(exp)) throw std::runtime_error(std::string("error at File: ")             \
        + __FILE__ + " Line: " + std::to_string(__LINE__) + " Statement: " + #exp);

template<class T>
constexpr size_t MaxEnumStringViewSize() {
	size_t maxsize = 0;
	for (const auto& strview : magic_enum::enum_names<T>()) {
		maxsize = maxsize < strview.size() ? strview.size() : maxsize;
	}
	return maxsize;
}

template<class Key, class Value>
Value GetOrDefault(const std::unordered_map<Key, std::any>& m, const Key& key, const Value& default_value) {
	auto itr = m.find(key);
	R_ASSERT(itr == m.end() || itr->second.type() == typeid(Value));
	return itr == m.end() ? default_value : std::any_cast<Value>(itr->second);
}

struct ScopeImGuiId {
	ScopeImGuiId(const char* id) {
		ImGui::PushID(id);
	}
	~ScopeImGuiId() {
		ImGui::PopID();
	}
};

struct ScopeImGuiGroup {
	ScopeImGuiGroup() {
		ImGui::BeginGroup();
	}
	~ScopeImGuiGroup() {
		ImGui::EndGroup();
	}
};

class ScopeImGuiCombo {
public:
	ScopeImGuiCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) {
		b = ImGui::BeginCombo(label, preview_value, flags);
	}
	~ScopeImGuiCombo() {
		if (b) ImGui::EndCombo();
	}
	operator bool() {
		return b;
	}
private:
	bool b{};
};

class ScopeImGuiTreeNode {
public:
	ScopeImGuiTreeNode(const char* label) {
		b = ImGui::TreeNode(label);
	}
	~ScopeImGuiTreeNode() {
		if (b) ImGui::TreePop();
	}
	operator bool() {
		return b;
	}
private:
	bool b{};
};

class ScopeImGuiPopupContextItem {
public:
	ScopeImGuiPopupContextItem(const char* str_id = NULL, ImGuiPopupFlags popup_flags = 1) {
		b = ImGui::BeginPopupContextItem(str_id, popup_flags);
	}
	~ScopeImGuiPopupContextItem() {
		if (b) ImGui::EndPopup();
	}
	operator bool() {
		return b;
	}
private:
	bool b{};
};
