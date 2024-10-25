#pragma once
#include "../lua/luau.h"
#include "../lua/lua_converters.h"

#include <utils/type_name.h>

#include <entt/core/hashed_string.hpp>

#include <unordered_map>
#include <list>

namespace dibidab::ecs
{
    class EventEmitter
    {
      public:

        template<typename EvenType>
        void emit(const EvenType &event, const char *customEventName = nullptr)
        {
            static entt::hashed_string::hash_type typeHash = entt::hashed_string { typename_utils::getTypeName<EvenType>().c_str() }.value();

            auto &listeners = eventListeners[customEventName ? entt::hashed_string { customEventName }.value() : typeHash];
            auto it = listeners.begin();

            bool removeListener = false;
            auto removeCallback = [&]
            {
                removeListener = true;
            };

            // call each listener with the event as argument:
            // also pass a callback function that can be used to remove the listener
            while (it != listeners.end())
            {
                auto &listener = *it;

                sol::protected_function_result result;

                // Pass event as reference if size is more than a pointer.
                if constexpr (sizeof(EvenType) > sizeof(size_t))
                {
                    // TODO: lua function might do stuff that breaks stuff, like it did in LuaScriptsSystem::callUpdateFunc()
                    result = listener(&event, removeCallback);
                }
                else
                {
                    result = listener(event, removeCallback);
                }

                if (!result.valid())
                {
                    throw gu_err(result.get<sol::error>().what());
                }

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

      private:
        std::unordered_map<entt::hashed_string::hash_type, std::list<sol::function>> eventListeners;
    };
}
