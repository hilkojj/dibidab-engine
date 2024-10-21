#include "EntityTemplate.h"
#include "../EntityEngine.h"

const std::string &EntityTemplate::getDescription() const
{
    return "";
}

entt::entity EntityTemplate::create(bool persistent)
{
    entt::entity e = engine->entities.create();
    createComponents(e, persistent);
    return e;
}
