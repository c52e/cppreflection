#pragma once

#define FIELD_NOT_FOUND_HANDLE(msg) throw std::runtime_error(msg);
#include <reflection/autoimgui_ext_glm.h>
#include <reflection/serialization_ext_glm.h>

using reflection::IAutoImGui;
using reflection::ISerialization;

using reflection::Deserialize;
using reflection::DrawAutoImGui;
using reflection::Serialize;
