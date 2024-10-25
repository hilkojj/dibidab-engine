#pragma once
#include <string>

namespace dibidab::ecs
{
    class Engine;

    /**
     * Base class for all entity systems.
     *
     * An entity system should be used for updating components of entities.
     */
    class System
    {
      public:
        System(const std::string &name);

        const std::string name;
        bool bUpdatesEnabled = true;

      protected:
        friend Engine;

        int updateFrequency = 0; // update this system n times per second. if n = 0 then update(deltaTime) is called, else update(1/n)
        float updateAccumulator = 0;

        virtual void init(Engine *) {}

        virtual void update(double deltaTime, Engine *) = 0;

        virtual ~System() = default;
    };
}
