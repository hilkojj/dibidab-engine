#include "luau.h"

#include "../reflection/StructInfo.h"
#include "../behavior/Tree.h"
#include "../level/Level.h"
#include "../dibidab/dibidab.h"

#include <input/gamepad_input.h>
#include <gu/game_utils.h>

luau::Script::Script(const std::string &path) : path(path)
{}

const sol::bytecode &luau::Script::getByteCode()
{
    if (!bytecode.empty())
    {
        return bytecode;
    }

    sol::load_result lr = luau::getLuaState().load_file(path);
    if (!lr.valid())
    {
        throw gu_err("Lua code invalid!:\n" + std::string(lr.get<sol::error>().what()));
    }

    bytecode = sol::protected_function(lr).dump();
    return bytecode;
}


template<typename type, typename vecType>
void populateVecUserType(sol::usertype<vecType> &vus)
{
    vus[sol::meta_function::multiplication] = [] (const vecType &a, const vecType &b)
    {
        return a * b;
    };
    vus[sol::meta_function::addition] = [] (const vecType &a, const vecType &b)
    {
        return a + b;
    };
    vus[sol::meta_function::subtraction] = [] (const vecType &a, const vecType &b)
    {
        return a - b;
    };
    vus[sol::meta_function::division] = [] (const vecType &a, const vecType &b)
    {
        for (int i = 0; i < vecType::length(); i++)
            if (b[i] == 0)
                throw gu_err("Division by zero. a = " + to_string(a) + ", b = " + to_string(b));
        return a / b;
    };
    vus[sol::meta_function::equal_to] = [] (const vecType &a, const vecType &b)
    {
        return a == b;
    };
    if constexpr (std::is_same_v<float, type>)
    {
        vus["length"] = [] (const vecType &v) {
            return length(v);
        };
    }

    vus[sol::meta_function::to_string] = [] (const vecType &a)
    {
        return to_string(a);
    };
}

template<typename type, typename vec2Type = vec<2, type, defaultp>, typename vec3Type = vec<3, type, defaultp>, typename vec4Type = vec<4, type, defaultp>>
void registerVecUserType(const std::string &name, sol::state &lua)
{
    sol::usertype<vec2Type> v2 = lua.new_usertype<vec2Type>(
        name + "2",
        sol::call_constructor,
        sol::constructors<
            vec2Type(), vec2Type(type), vec2Type(type, type), vec2Type(vec2Type), vec2Type(vec<2, int, defaultp>), vec2Type(vec<2, float, defaultp>)
        >()
    );
    v2["x"] = &vec2Type::x;
    v2["y"] = &vec2Type::y;
    populateVecUserType<type>(v2);

    sol::usertype<vec3Type> v3 = lua.new_usertype<vec3Type>(
        name + "3",
        sol::call_constructor,
        sol::constructors<
            vec3Type(), vec3Type(type), vec3Type(type, type, type), vec3Type(vec3Type), vec3Type(vec<3, int, defaultp>), vec3Type(vec<3, float, defaultp>)
        >()
    );
    v3["x"] = &vec3Type::x;
    v3["r"] = &vec3Type::x;
    v3["y"] = &vec3Type::y;
    v3["g"] = &vec3Type::y;
    v3["z"] = &vec3Type::z;
    v3["b"] = &vec3Type::z;
    populateVecUserType<type>(v3);

    sol::usertype<vec4Type> v4 = lua.new_usertype<vec4Type>(
        name + "4",
        sol::call_constructor,
        sol::constructors<
            vec4Type(), vec4Type(type), vec4Type(type, type, type, type), vec4Type(vec4Type), vec4Type(vec<4, int, defaultp>), vec4Type(vec<4, float, defaultp>
        )>()
    );
    v4["x"] = &vec4Type::x;
    v4["r"] = &vec4Type::x;
    v4["y"] = &vec4Type::y;
    v4["g"] = &vec4Type::y;
    v4["z"] = &vec4Type::z;
    v4["b"] = &vec4Type::z;
    v4["w"] = &vec4Type::w;
    v4["a"] = &vec4Type::w;
    populateVecUserType<type>(v4);
}


sol::state &luau::getLuaState()
{
    static sol::state *lua = nullptr;

    if (lua == nullptr)
    {
        lua = new sol::state;
        lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

        auto &env = lua->globals();

        env["getGameStartupArgs"] = []
        {
            return &dibidab::startupArgs;
        };
        env["tryCloseGame"] = []
        {
            gu::setShouldClose(true);
        };

        env["loadOrCreateLevel"] = [] (const sol::optional<std::string> &path)
        {
            dibidab::setLevel(path.has_value() ? new dibidab::level::Level(path.value().c_str()) : nullptr);
        };

        env["include"] = [&] (const char *scriptPath, const sol::this_environment &currentEnv) -> sol::environment
        {
            auto newEnv = sol::environment(getLuaState(), sol::create, currentEnv.env.value_or(env));

            asset<Script> toBeIncluded(scriptPath);
            getLuaState().unsafe_script(toBeIncluded->getByteCode().as_string_view(), newEnv);
            return newEnv;
        };

        // register dibidab headers:
        for (const auto &[name, structInfo] : dibidab::getAllStructInfos())
        {
            if (structInfo.registerLuaUserType)
            {
                structInfo.registerLuaUserType(*lua);
            }
        }

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
        {
            qut[axis == 0 ? "x" : (axis == 1 ? "y" : "z")] = sol::property(
                [axis] (quat &q)
                {
                    return glm::eulerAngles(q)[axis] * mu::RAD_TO_DEGREES;
                },
                [axis] (quat &q, float x)
                {
                    vec3 euler = glm::eulerAngles(q);
                    euler[axis] = x * mu::DEGREES_TO_RAD;
                    q = quat(euler);
                }
            );
        }
        qut["setIdentity"] = [] (quat &q) -> quat &
        {
            q = quat(1, 0, 0, 0);
            return q;
        };
        qut["getAngle"] = [] (quat &q) -> float { return angle(q) * mu::RAD_TO_DEGREES; };
        qut["getAxis"] = [] (quat &q) -> vec3 { return axis(q); };
        qut["setFromAngleAndAxis"] = [] (quat &q, float angle, vec3 axis)
        {
            q = angleAxis(angle * mu::DEGREES_TO_RAD, axis);
        };

        // register KeyInput::Key
        sol::usertype<KeyInput::Key> key = lua->new_usertype<KeyInput::Key>("Key");
        key["getName"] = [] (KeyInput::Key &key)
        {
            return KeyInput::getKeyName(key);
        };

        // register GamepadInput::Button
        sol::usertype<GamepadInput::Button> gpb = lua->new_usertype<GamepadInput::Button>("GamepadButton");
        gpb["getName"] = [] (GamepadInput::Button &key)
        {
            return GamepadInput::getButtonName(key);
        };

        // register GamepadInput::Axis
        sol::usertype<GamepadInput::Axis> gpa = lua->new_usertype<GamepadInput::Axis>("GamepadAxis");
        gpa["getName"] = [] (GamepadInput::Axis &key)
        {
            return GamepadInput::getAxisName(key);
        };

        dibidab::behavior::Tree::addToLuaEnvironment(lua);
    }
    return *lua;
}

sol::environment luau::environmentFromScript(luau::Script &script, sol::environment *parent)
{
    sol::environment env = parent ? sol::environment(getLuaState(), sol::create, *parent)
        : sol::environment(getLuaState(), sol::create, getLuaState().globals());

    sol::protected_function_result result = luau::getLuaState().safe_script(script.getByteCode().as_string_view(), env);
    if (!result.valid())
    {
        throw gu_err(result.get<sol::error>().what());
    }

    return env;
}
