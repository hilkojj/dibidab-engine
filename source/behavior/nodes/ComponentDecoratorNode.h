#pragma once

#include "../Tree.h"

#include "../../reflection/ComponentInfo.h"
#include "../../ecs/Engine.h"

namespace dibidab::behavior
{
    struct ComponentDecoratorNode : public Tree::DecoratorNode
    {
        template<class Component>
        ComponentDecoratorNode *addWhileEntered(ecs::Engine *engine, entt::entity entity)
        {
            addWhileEntered(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void addWhileEntered(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

        template<class Component>
        ComponentDecoratorNode *addOnEnter(ecs::Engine *engine, entt::entity entity)
        {
            addOnEnter(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void addOnEnter(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

        template<class Component>
        ComponentDecoratorNode *removeOnFinish(ecs::Engine *engine, entt::entity entity)
        {
            removeOnFinish(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void removeOnFinish(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

        template<class Component>
        ComponentDecoratorNode *removeOnEnter(ecs::Engine *engine, entt::entity entity)
        {
            removeOnEnter(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void removeOnEnter(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

        void enter() override;

        void finish(Result result) override;

        const char *getName() const override;

        void drawDebugInfo() const override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:

        struct EntityComponent
        {
            ecs::Engine *engine = nullptr;
            entt::entity entity = entt::null;
            const ComponentInfo *component = nullptr;
        };

        std::vector<EntityComponent> toAddWhileEntered;
        std::vector<EntityComponent> toAddOnEnter;
        std::vector<EntityComponent> toRemoveOnFinish;
        std::vector<EntityComponent> toRemoveOnEnter;
    };
}
