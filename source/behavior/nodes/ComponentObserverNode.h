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

        template<class Component>
        ComponentObserverNode *has(ecs::Engine *engine, entt::entity entity)
        {
            has(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void has(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

        template<class Component>
        ComponentObserverNode *exclude(ecs::Engine *engine, entt::entity entity)
        {
            exclude(engine, entity, dibidab::findComponentInfo<Component>());
            return this;
        }

        void exclude(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component);

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

        void observe(ecs::Engine *engine, entt::entity entity, const ComponentInfo *component, bool presentValue, bool absentValue);

        bool allConditionsFulfilled() const;

        void onConditionsChanged(ecs::Engine *engine, entt::entity entity);

        int getChildIndexToEnter() const;

        void enterChild();

        struct ObserverHandle
        {
            ecs::Engine *engine = nullptr;
            const ComponentInfo *component = nullptr;
            ecs::Observer::Handle handle;
        };

        std::vector<ObserverHandle> observerHandles;
        std::vector<bool> conditions;
        bool bFulFilled = false;
        delegate_method switchBranchNextUpdate;

        int fulfilledNodeIndex = INVALID_CHILD_INDEX;
        int unfulfilledNodeIndex = INVALID_CHILD_INDEX;
        int currentNodeIndex = INVALID_CHILD_INDEX;

        friend class TreeInspector;
    };
}
