
#include "game/dibidab.h"
#include "dibidab/struct.h"

#include "game/dibidab_settings.dibidab.h"

#include "generated/registry.struct_json.h"

#include "ecs/components/Children.dibidab.h"

#include <gu/game_config.h>

int main(int argc, char *argv[])
{
    gu::Config configgy;
    dibidab::init(argc, argv, configgy);

    std::cout << dibidab::findStructInfo("dibidab::EngineSettings")->id << std::endl;

    Parent parent;
    json parentJson = parent;

    /*
    dibidab::findStructInfo("dibidab::EngineSettings")->registerLuaUserType(luau::getLuaState());

    luau::getLuaState().safe_script(R"(
        _G.test = EngineSettings {
            bShowDeveloperOptions = true
        }
        print(test.bShowDeveloperOptions)
    )", sol::script_throw_on_error);
    dibidab::EngineSettings test = luau::getLuaState()["test"];
    json jsonObj;
    dibidab::to_json_object(jsonObj, test);
    std::cout << jsonObj.dump() << std::endl;
     */

    dibidab::run();
    return 0;
}
