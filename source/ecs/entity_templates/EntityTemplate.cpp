#include "EntityTemplate.h"
#include "../EntityEngine.h"

const std::string &EntityTemplate::getDescription()
{
    static const std::string defaultDescription = "";
    return defaultDescription;
}

entt::entity EntityTemplate::create(bool persistent)
{
    entt::entity e = engine->entities.create();
    createComponents(e, persistent);
    return e;
}
