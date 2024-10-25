#pragma once
#include "System.h"
#include "../Engine.h"

#include <utils/delegate.h>

namespace dibidab::ecs
{
    class TimeOutSystem : public System
    {
        using System::System;

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
        void init(Engine *engine) override;

        void update(double deltaTime, Engine *engine) override;

      private:
        Engine *engine = nullptr;
    };
}
