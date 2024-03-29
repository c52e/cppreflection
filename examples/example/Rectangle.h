#pragma once

#include "Shape.h"

class Rectangle : public Shape {
public:
    float w{};
    float h{};

    FIELD_DECLARATION_BEGIN(ISerialization)
    FIELD_DECLARATION("w", w)
    FIELD_DECLARATION("h", h)
    FIELD_DECLARATION_END_WITH_BASE_CLASS(Shape)
    FIELD_DECLARATION_BEGIN(IAutoImGui)
    FIELD_DECLARATION("w", w)
    FIELD_DECLARATION("h", h)
    FIELD_DECLARATION_END_WITH_BASE_CLASS(Shape)
};
