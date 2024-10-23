#pragma once
#include <dibidab_header.h>
#include <json.hpp>

struct EntityTemplateArgs
{
  dibidab_expose(json);
    json createFunctionArguments = json::object();
};
