#include "Template.h"

#include "../Engine.h"

const std::string &dibidab::ecs::Template::getDescription() const
{
    static std::string description = "";
    return description;
}

entt::entity dibidab::ecs::Template::create(bool bPersistent)
{
    const entt::entity e = engine->entities.create();
    createComponents(e, bPersistent);
    return e;
}
