#pragma once
#include "EntityTemplate.h"

#include <utility>
#include "../../level/room/Room.h"
#include "lua/luau.h"
#include "ecs/components/Saving.dibidab.h"

namespace dibidab::ecs
{
    class LuaEntityTemplate : public EntityTemplate
    {
      public:
        const std::string name;

        asset<luau::Script> script;

        LuaEntityTemplate(const char *assetName, const char *name, EntityEngine *);

        const std::string &getDescription() const override;

        json getDefaultArgs();

        void createComponents(entt::entity, bool persistent) override;

        void createComponentsWithJsonArguments(entt::entity, const json &arguments, bool persistent);

        void createComponentsWithLuaArguments(entt::entity, sol::optional<sol::table> arguments, bool persistent);

        sol::environment &getTemplateEnvironment();

      protected:
        void runScript();

        std::string getUniqueID();

      private:
        std::string description;
        sol::table defaultArgs;

        sol::environment luaEnvironment;
        sol::safe_function luaCreateComponents;

        Persistent persistency;
        bool bPersistentArgs = false;
    };
}
