#pragma once

#include "Shape.h"

class Circle : public Shape {
public:
    float r{};

    FIELD_DECLARATION_BEGIN(ISerialization)
        FIELD_DECLARATION("r", r)
    FIELD_DECLARATION_END_WITH_BASE_CLASS(Shape)
    FIELD_DECLARATION_BEGIN(IAutoImGui)
        FIELD_DECLARATION("r", r, d.Min = 0.0f, d.Max = 100.0f)
    FIELD_DECLARATION_END_WITH_BASE_CLASS(Shape)
};
