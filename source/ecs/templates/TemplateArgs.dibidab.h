#pragma once
#include <dibidab_header.h>
#include <json.hpp>

namespace dibidab::ecs
{
    struct TemplateArgs
    {
        dibidab_expose(json);
        json createFunctionArguments = json::object();
    };
}
