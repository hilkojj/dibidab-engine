#pragma once

#include "../Tree.h"

#include <utils/delegate.h>

#include <entt/entity/fwd.hpp>

namespace dibidab::ecs
{
    class Engine;
}

namespace dibidab::behavior
{
    struct WaitNode : public Tree::LeafNode
    {
        WaitNode();

        WaitNode *finishAfter(float seconds, entt::entity waitingEntity, ecs::Engine *engine);

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        const char *getName() const override;

        void drawDebugInfo() const override;

      private:
        /**
         * If set to >= 0, will abort in n frames (where n could be 0, in case `seconds` <= deltaTime).
         * Else, will only abort if parent aborts.
         */
        float seconds;
        /**
         * Can be any entity. If the entity gets destroyed while waiting, abort() will only be called if the parent aborts.
         */
        entt::entity waitingEntity;
        ecs::Engine *engine;

        delegate_method onWaitFinished;

#ifndef NDEBUG
        float timeStarted;
#endif
    };
}
