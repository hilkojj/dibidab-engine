#pragma once
#include "EntitySystem.h"
#include "../EntityEngine.h"

#include <utils/delegate.h>

namespace dibidab::ecs
{
    class TimeOutSystem : public EntitySystem
    {
        using EntitySystem::EntitySystem;

      public:

        /**
         * 'Unsafe' as in: This function will assign the TimeOuts component if not present. Which is behavior that
         * can cause crashes if this function is called from somewhere where assigning Components is not allowed:
         *  - Entity is being destroyed.
         *  - ???
         */
        delegate_method unsafeCallAfter(float seconds, entt::entity waitingEntity, const std::function<void()> &callback);

        delegate<void()> nextUpdate;

      protected:
        void init(EntityEngine *engine) override;

        void update(double deltaTime, EntityEngine *engine) override;

      private:
        EntityEngine *engine = nullptr;
    };
}
