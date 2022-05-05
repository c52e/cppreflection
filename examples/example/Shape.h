#pragma once

#include "reflection_common.h"

class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape() {};
};

HAS_SUBCLASS(Shape)
