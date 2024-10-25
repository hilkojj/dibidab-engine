#include "ComponentDecoratorNode.h"

#include <imgui.h>

void dibidab::behavior::ComponentDecoratorNode::addWhileEntered(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getReadableDebugInfo());
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getReadableDebugInfo());
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getReadableDebugInfo());
    }
    toAddWhileEntered.push_back({engine, entity, component});
}

void dibidab::behavior::ComponentDecoratorNode::addOnEnter(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getReadableDebugInfo());
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getReadableDebugInfo());
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getReadableDebugInfo());
    }
    toAddOnEnter.push_back({engine, entity, component});
}

void dibidab::behavior::ComponentDecoratorNode::removeOnFinish(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getReadableDebugInfo());
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getReadableDebugInfo());
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getReadableDebugInfo());
    }
    toRemoveOnFinish.push_back({engine, entity, component});
}

void dibidab::behavior::ComponentDecoratorNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("ComponentDecoratorNode has no child: " + getReadableDebugInfo());
    }
    Node::enter();

    for (const std::vector<EntityComponent> &toAddVector : { toAddWhileEntered, toAddOnEnter })
    {
        for (const EntityComponent entityComponent : toAddVector)
        {
            if (!entityComponent.engine->entities.valid(entityComponent.entity))
            {
                throw gu_err("Cannot add " + std::string(entityComponent.component->name) +
                    " to an invalid entity while entering: " + getReadableDebugInfo());
            }
            entityComponent.component->addComponent(entityComponent.entity, entityComponent.engine->entities);
        }
    }

    child->enter();
}

void dibidab::behavior::ComponentDecoratorNode::finish(Result result)
{
    for (const std::vector<EntityComponent> &entityComponentsToRemove : { toAddWhileEntered, toRemoveOnFinish })
    {
        for (const EntityComponent entityComponent : entityComponentsToRemove)
        {
            if (!entityComponent.engine->entities.valid(entityComponent.entity))
            {
                continue; // Ignore.
            }
            entityComponent.component->removeComponent(entityComponent.entity, entityComponent.engine->entities);
        }
    }
    DecoratorNode::finish(result);
}

const char *dibidab::behavior::ComponentDecoratorNode::getName() const
{
    return "ComponentDecorator";
}

void dibidab::behavior::ComponentDecoratorNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
    for (int i = 0; i < toAddWhileEntered.size(); i++)
    {
        if (i > 0)
        {
            ImGui::TextDisabled(" | ");
            ImGui::SameLine();
        }
        const EntityComponent &entityComponent = toAddWhileEntered[i];

        ImGui::TextDisabled("Add ");
        ImGui::SameLine();
        ImGui::Text("%s", entityComponent.component->name);
        ImGui::SameLine();
        ImGui::TextDisabled(" to ");
        ImGui::SameLine();

        if (!entityComponent.engine->entities.valid(entityComponent.entity))
        {
            ImGui::Text("Invalid entity #%d", int(entityComponent.entity));
            ImGui::SameLine();
        }
        else if (const char *entityName = entityComponent.engine->getName(entityComponent.entity))
        {
            ImGui::Text("#%d %s", int(entityComponent.entity), entityName);
            ImGui::SameLine();
        }
        else
        {
            ImGui::Text("#%d", int(entityComponent.entity));
            ImGui::SameLine();
        }
    }
}

void dibidab::behavior::ComponentDecoratorNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    finish(result);
}
