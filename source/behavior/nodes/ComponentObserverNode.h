#pragma once

#include "../Tree.h"

#include "../../reflection/ComponentInfo.h"
#include "../../ecs/Observer.h"

#include <utils/delegate.h>

namespace dibidab::ecs
{
    class Engine;
}

namespace dibidab::behavior
{
    struct ComponentObserverNode : public Tree::CompositeNode
    {
        ComponentObserverNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        ComponentObserverNode *withoutSafetyDelay();

        template<class Component>
        ComponentObserverNode *has(ecs::Engine *engine, entt::entity entity)
        {
            has(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void has(ecs::Engine *engine, entt::entity entity, const dibidab::ComponentInfo *component);

        template<class Component>
        ComponentObserverNode *exclude(ecs::Engine *engine, entt::entity entity)
        {
            exclude(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void exclude(ecs::Engine *engine, entt::entity entity, const dibidab::ComponentInfo *component);

        ComponentObserverNode *setOnFulfilledNode(Node *child);

        ComponentObserverNode *setOnUnfulfilledNode(Node *child);

        // Do not use:
        CompositeNode *addChild(Node *child) override;

        const char *getName() const override;

        void drawDebugInfo() const override;

        ~ComponentObserverNode() override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:

        void observe(ecs::Engine *engine, entt::entity entity, const dibidab::ComponentInfo *component, bool presentValue, bool absentValue);

        bool allConditionsFulfilled() const;

        void onConditionsChanged(ecs::Engine *engine, entt::entity entity);

        int getChildIndexToEnter() const;

        void enterChild();

        struct ObserverHandle
        {
            ecs::Engine *engine = nullptr;
            const ComponentInfo *component = nullptr;
            ecs::Observer::Handle handle;
            delegate_method latestConditionChangedDelay;
        };

        std::vector<ObserverHandle> observerHandles;
        std::vector<bool> conditions;
        bool bUseSafetyDelay;
        int fulfilledNodeIndex;
        int unfulfilledNodeIndex;
        bool bFulFilled;
        int currentNodeIndex;

        friend class TreeInspector;
    };
}
