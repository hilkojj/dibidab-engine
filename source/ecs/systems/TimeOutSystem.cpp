#include "TimeOutSystem.h"

struct TimeOuts
{
    std::list<std::pair<float, delegate<void()>>> timeOutList;
};

delegate_method dibidab::ecs::TimeOutSystem::unsafeCallAfter(float seconds, entt::entity waitingEntity,
    const std::function<void()> &callback)
{
    if (!engine)
    {
        throw gu_err("No engine set! Has the system initialized yet?");
    }
    if (!engine->entities.valid(waitingEntity))
    {
        throw gu_err("Waiting entity #" + std::to_string(int(waitingEntity)) + " is not valid!");
    }
    TimeOuts &timeOuts = engine->entities.get_or_assign<TimeOuts>(waitingEntity);
    std::pair<float, delegate<void()>> &timeOut = timeOuts.timeOutList.emplace_back(seconds, delegate<void()>());
    return timeOut.second += callback;
}

void dibidab::ecs::TimeOutSystem::init(Engine *inEngine)
{
    System::init(inEngine);
    engine = inEngine;
}

void dibidab::ecs::TimeOutSystem::update(double deltaTimeDouble, Engine *)
{
    nextUpdate();
    nextUpdate = delegate<void()>();

    /*
     * NOTE: it is important that we call callbacks in order of adding them, in case they both need to be called in the same frame.
     * So no swap and pop here for example!
     */

    float deltaTime = float(deltaTimeDouble);
    std::list<std::pair<entt::entity, delegate<void()>>> toCall;
    engine->entities.view<TimeOuts>().each([&] (entt::entity e, TimeOuts &timeOuts)
    {
        auto it = timeOuts.timeOutList.begin();
        while (it != timeOuts.timeOutList.end())
        {
            auto &[seconds, delegate] = *it;
            seconds -= deltaTime;
            if (seconds <= 0.0f)
            {
                /*
                 * We're copying the delegate, so that the callbacks can do all sorts of funny things without affecting
                 * the current view, to prevent undefined behavior.
                 *
                 * https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system#what-is-allowed-and-what-is-not
                 */
                toCall.emplace_back(e, delegate);
                timeOuts.timeOutList.erase(it++);
            }
            else
            {
                it++;
            }
        }
        if (timeOuts.timeOutList.empty())
        {
            engine->entities.remove<TimeOuts>(e);
        }
    });
    for (auto &[entity, delegate] : toCall)
    {
        if (engine->entities.valid(entity))
        {
            delegate();
        }
    }
}
