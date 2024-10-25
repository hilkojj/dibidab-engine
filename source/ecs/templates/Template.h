#pragma once

#include <entt/entity/fwd.hpp>

#include <string>

namespace dibidab::ecs
{
    class Engine;

    /**
     * Abstract class.
     *
     * Entity templates are used to construct one (or more) entities with a collection of components.
     */
    class Template
    {
      private:
        friend class Engine;

        int templateHash = -1;

      protected:
        Engine *engine = nullptr;

      public:

        virtual const std::string &getDescription() const;

        entt::entity create(bool persistent = false);

        virtual void createComponents(entt::entity, bool persistent = false) = 0;

      protected:

        virtual ~Template() = default;

    };
}
