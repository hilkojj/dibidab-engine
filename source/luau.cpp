
#include "game/session/SingleplayerSession.h"
#include "luau.h"
#include "game/dibidab.h"

template<typename type, typename vec2Type = vec<2, type, defaultp>, typename vec3Type = vec<3, type, defaultp>, typename vec4Type = vec<4, type, defaultp>>
void registerVecUserType(const std::string &name, sol::state &lua)
{
    sol::usertype<vec2Type> v2 = lua.new_usertype<vec2Type>(name + "2", sol::call_constructor,

        sol::constructors<vec2Type(), vec2Type(type), vec2Type(type, type)>()
    );
    v2["x"] = &vec2Type::x;
    v2["y"] = &vec2Type::y;

    sol::usertype<vec3Type> v3 = lua.new_usertype<vec3Type>(name + "3", sol::call_constructor,

        sol::constructors<vec3Type(), vec3Type(type), vec3Type(type, type, type)>()
    );
    v3["x"] = &vec3Type::x;
    v3["r"] = &vec3Type::x;
    v3["y"] = &vec3Type::y;
    v3["g"] = &vec3Type::y;
    v3["z"] = &vec3Type::z;
    v3["b"] = &vec3Type::z;
    sol::usertype<vec4Type> v4 = lua.new_usertype<vec4Type>(name + "4", sol::call_constructor,

        sol::constructors<vec4Type(), vec4Type(type), vec4Type(type, type, type, type)>()
    );
    v4["x"] = &vec4Type::x;
    v4["r"] = &vec4Type::x;
    v4["y"] = &vec4Type::y;
    v4["g"] = &vec4Type::y;
    v4["z"] = &vec4Type::z;
    v4["b"] = &vec4Type::z;
    v4["w"] = &vec4Type::w;
    v4["a"] = &vec4Type::w;
}


sol::state &luau::getLuaState()
{
    static sol::state *lua = NULL;

    if (lua == NULL)
    {
        lua = new sol::state;
        lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

        auto &env = lua->globals();

        // math utils lol: todo: make this work with vec types instead
        env["rotate2d"] = [&](float x, float y, float degrees) -> sol::table {
            auto table = sol::table::create(env.lua_state());

            auto result = rotate(vec2(x, y), degrees * mu::DEGREES_TO_RAD);
            table[1] = result.x;
            table[2] = result.y;
            return table;
        };

        env["printTableAsJson"] = [&] (const sol::table &table, sol::optional<int> indent) // todo: this doest work properly
        {
            json j;
            jsonFromLuaTable(table, j);
            std::cout << j.dump(indent.has_value() ? indent.value() : -1) << std::endl;
        };

        env["getGameStartupArgs"] = [] {
            return &dibidab::startupArgs;
        };
        env["tryCloseGame"] = [] {
            gu::setShouldClose(true);
        };

        env["endCurrentSession"] = [] {
            dibidab::setCurrentSession(NULL);
        };

        env["startSinglePlayerSession"] = [] (const char *saveGamePath) {
            dibidab::setCurrentSession(new SingleplayerSession(saveGamePath));
        };
        // todo: startMultiplayerServerSession and startMultiplayerClientsession

        env["joinSession"] = [] (const char *username, const sol::function& onJoinRequestDeclined) {

            auto &session = dibidab::getCurrentSession();

            session.onJoinRequestDeclined = [onJoinRequestDeclined] (auto reason) {

                sol::protected_function_result result = onJoinRequestDeclined(reason);
                if (!result.valid())
                    throw gu_err(result.get<sol::error>().what());
            };
            session.join(username);
        };

        env["loadOrCreateLevel"] = [](const char *path)
        {
            auto &session = dibidab::getCurrentSession();
            auto singleplayerSession = dynamic_cast<SingleplayerSession *>(&session);
            if (singleplayerSession)
                singleplayerSession->setLevel(new Level(path));
        };

        env["include"] = [&] (const char *scriptPath, const sol::this_environment &currentEnv) -> sol::environment {

            auto newEnv = sol::environment(getLuaState(), sol::create, currentEnv.env.value_or(env));

            asset<Script> toBeIncluded(scriptPath);
            getLuaState().unsafe_script(toBeIncluded->getByteCode().as_string_view(), newEnv);
            return newEnv;
        };

        // register Yaml-structs:
        for (auto &[typeName, info] : SerializableStructInfo::getForAllTypes())
            info->luaUserTypeGenerator(*lua);

        // register glm vectors:
        registerVecUserType<int>("ivec", *lua);
        registerVecUserType<int8>("i8vec", *lua);
        registerVecUserType<int16>("i16vec", *lua);
        registerVecUserType<uint>("uvec", *lua);
        registerVecUserType<uint8>("u8vec", *lua);
        registerVecUserType<uint16>("u16vec", *lua);
        registerVecUserType<float>("vec", *lua);

        // register glm quat:
        sol::usertype<quat> qut = lua->new_usertype<quat>("quat");

        for (int axis = 0; axis < 3; axis++)
            qut[axis == 0 ? "x" : (axis == 1 ? "y" : "z")] = sol::property([axis](quat &q) {
                return glm::eulerAngles(q)[axis] * mu::RAD_TO_DEGREES;
            }, [axis](quat &q, float x) {
                vec3 euler = glm::eulerAngles(q);
                euler[axis] = x * mu::DEGREES_TO_RAD;
                q = quat(euler);
            });

        qut["getAngle"] = [] (quat &q) -> float { return angle(q) * mu::RAD_TO_DEGREES; };
        qut["getAxis"] = [] (quat &q) -> vec3 { return axis(q); };
        qut["setFromAngleAndAxis"] = [] (quat &q, float angle, vec3 axis) {
            q = angleAxis(angle * mu::DEGREES_TO_RAD, axis);
        };
    }
    return *lua;
}

luau::Script::Script(const std::string &path) : path(path)
{}

const sol::bytecode &luau::Script::getByteCode()
{
    if (!bytecode.empty())
        return bytecode;

    sol::load_result lr = luau::getLuaState().load_file(path);
    if (!lr.valid())
        throw gu_err("Lua code invalid!");

    bytecode = sol::protected_function(lr).dump();
    return bytecode;
}
