#pragma once
#include "Template.h"

#include "../components/Persistent.dibidab.h"

#include "../../level/room/Room.h"
#include "../../lua/luau.h"

namespace dibidab::ecs
{
    class LuaTemplate : public Template
    {
      public:
        const std::string name;

        asset<luau::Script> script;

        LuaTemplate(const char *assetName, const char *name, Engine *);

        const std::string &getDescription() const override;

        json getDefaultArgs();

        void createComponents(entt::entity, bool bPersistent) override;

        void createComponentsWithJsonArguments(entt::entity, const json &arguments, bool bPersistent);

        void createComponentsWithLuaArguments(entt::entity, sol::optional<sol::table> arguments, bool bPersistent);

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
