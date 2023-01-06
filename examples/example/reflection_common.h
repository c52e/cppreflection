#pragma once

#define FIELD_NOT_FOUND_HANDLE(msg) throw std::runtime_error(msg);
#include <reflection/serialization_ext_glm.h>
#include <reflection/autoimgui_ext_glm.h>

using reflection::ISerialization;
using reflection::IAutoImGui;

using reflection::Serialize;
using reflection::Deserialize;
using reflection::DrawAutoImGui;
