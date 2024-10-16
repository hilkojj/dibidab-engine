#pragma once
#include <unordered_map>
#include <list>
#include <utils/type_name.h>
#include "../../external/entt/src/entt/core/hashed_string.hpp"
#include "../luau.h"
#include "dibidab/converters/lua_converters.h"

class EventEmitter
{

    using hash_type = entt::hashed_string::hash_type;

    std::unordered_map<hash_type, std::list<sol::function>> eventListeners;

  public:

    template<typename type>
    void emit(const type &event, const char *customEventName=nullptr)
    {
        static hash_type typeHash = 0;
        if (typeHash == 0)
            typeHash = entt::hashed_string { typename_utils::getTypeName<type>().c_str() }.value();

        auto &listeners = eventListeners[customEventName ? entt::hashed_string { customEventName }.value() : typeHash];
        auto it = listeners.begin();

        bool removeListener = false;
        auto removeCallback = [&] {
            removeListener = true;
        };

        // call each listener with the event as argument:
        // also pass a callback function that can be used to remove the listener
        while (it != listeners.end())
        {
            auto &listener = *it;

            sol::protected_function_result result;

            if constexpr (sizeof(type) > 4 && !std::is_same_v<const char *, type>)
                result = listener(&event, removeCallback);   // TODO: lua function might do stuff that breaks stuff, like it did in LuaScriptsSystem::callUpdateFunc()
            else
                result = listener(event, removeCallback); // copy the value instead of giving a pointer.

            if (!result.valid())
                throw gu_err(result.get<sol::error>().what());

            if (removeListener)
            {
                removeListener = false;
                it = listeners.erase(it);
            }
            else ++it;
        }
    }

    void on(const char *eventName, const sol::function &listener)
    {
        auto typeHash = entt::hashed_string { eventName }.value();

        auto &listeners = eventListeners[typeHash];

        listeners.push_back(listener);
    }

};
