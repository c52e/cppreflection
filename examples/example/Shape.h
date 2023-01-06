#pragma once

#include "reflection_common.h"

class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape() {};

    int id{};

    FIELD_DECLARATION_BEGIN(ISerialization)
        FIELD_DECLARATION("id", id)
    FIELD_DECLARATION_END()
    FIELD_DECLARATION_BEGIN(IAutoImGui)
        FIELD_DECLARATION("id", id)
    FIELD_DECLARATION_END()
};

HAS_SUBCLASS(Shape)
