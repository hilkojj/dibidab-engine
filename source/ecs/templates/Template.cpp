#include "Template.h"

#include "../Engine.h"

const std::string &dibidab::ecs::Template::getDescription() const
{
    return "";
}

entt::entity dibidab::ecs::Template::create(bool persistent)
{
    entt::entity e = engine->entities.create();
    createComponents(e, persistent);
    return e;
}
