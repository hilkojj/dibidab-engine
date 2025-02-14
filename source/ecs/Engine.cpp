#include "Engine.h"

#include "Observer.h"

#include "systems/KeyEventsSystem.h"
#include "systems/TimeOutSystem.h"
#include "templates/LuaTemplate.h"
#include "components/Children.dibidab.h"
#include "components/LuaScripted.dibidab.h"

#include "../reflection/ComponentInfo.h"
#include "../reflection/StructInfo.h"

#include <assets/AssetManager.h>
#include <gu/profiler.h>
#include <utils/string_utils.h>
#include <utils/hashing.h>

void dibidab::ecs::Engine::addSystem(System *sys, bool pushFront)
{
    assert(!bInitialized);
    if (pushFront)
        systems.push_front(sys);
    else
        systems.push_back(sys);
}

std::list<dibidab::ecs::System *> dibidab::ecs::Engine::getSystems()
{
    return systems;
}

dibidab::ecs::TimeOutSystem *dibidab::ecs::Engine::getTimeOuts()
{
    return timeOutSystem;
}

const std::string &dibidab::ecs::Engine::getTemplateDirectoryPath() const
{
    return templateDirectoryPath;
}

dibidab::ecs::Template &dibidab::ecs::Engine::getTemplate(std::string name)
{
    try
    {
        return getTemplate(hashStringCrossPlatform(name));
    }
    catch (_gu_err &)
    {
        throw gu_err("No EntityTemplate named " + name + " found");
    }
}

dibidab::ecs::Template &dibidab::ecs::Engine::getTemplate(int templateHash)
{
    auto t = entityTemplates[templateHash];
    if (!t)
        throw gu_err("No EntityTemplate found for hash " + std::to_string(templateHash));
    return *t;
}

const std::vector<std::string> &dibidab::ecs::Engine::getTemplateNames() const
{
    return entityTemplateNames;
}

entt::entity dibidab::ecs::Engine::getChildByName(entt::entity parent, const char *childName)
{
    const Parent *p = entities.try_get<Parent>(parent);
    if (!p)
        return entt::null;
    auto it = p->childrenByName.find(childName);
    if (it == p->childrenByName.end())
        return entt::null;
    return it->second;
}

void dibidab::ecs::Engine::registerLuaEntityTemplate(const char *assetPath)
{
    auto name = su::split(assetPath, "/").back();
    if (su::startsWith(name, "_"))
        return;

    addEntityTemplate(name, new LuaTemplate(assetPath, name.c_str(), this));
}

void dibidab::ecs::Engine::addEntityTemplate(const std::string &name, Template *t)
{
    int hash = hashStringCrossPlatform(name);

    bool replace = entityTemplates[hash] != nullptr;

    delete entityTemplates[hash];
    auto et = entityTemplates[hash] = t;
    et->engine = this;
    et->templateHash = hash;

    if (!replace)
        entityTemplateNames.push_back(name);
}

dibidab::ecs::Engine::~Engine()
{
    bDestructing = true;
    for (System *system : systems)
    {
        delete system;
    }
    for (auto &templateEntry : entityTemplates)
    {
        delete templateEntry.second;
    }
    for (auto &[component, observer] : observerPerComponent)
    {
        delete observer;
    }
}

bool dibidab::ecs::Engine::isDestructing() const
{
    return bDestructing;
}

struct Named
{
    std::string name_dont_change;
};

void dibidab::ecs::Engine::initialize()
{
    assert(!bInitialized);

    addSystem(new KeyEventsSystem("Key Listeners"));
    timeOutSystem = new TimeOutSystem("Timeouts");
    addSystem(timeOutSystem);

    entities.on_construct<Child>().connect<&Engine::onChildCreation>(this);
    entities.on_destroy<Child>().connect<&Engine::onChildDeletion>(this);

    entities.on_destroy<Parent>().connect<&Engine::onParentDeletion>(this);

    entities.on_destroy<Named>().connect<&Engine::onEntityDenaming>(this);

    initializeLuaEnvironment();

    std::map<std::string, bool> registered;

    for (auto &el : AssetManager::getAssetsForType<luau::Script>())
    {
        auto shortPath = el.second->shortPath.c_str();
        if (registered[shortPath])
            continue;

        if (su::startsWith(el.first, templateDirectoryPath))
        {
            registerLuaEntityTemplate(shortPath);
            registered[shortPath] = true;
        }
    }

    for (auto sys : systems)
        sys->init(this);

    bInitialized = true;
}

void dibidab::ecs::Engine::initializeLuaEnvironment()
{
    // todo: functions might be called after Engine is destructed

    luaEnvironment = sol::environment(luau::getLuaState(), sol::create, luau::getLuaState().globals());
    auto &env = luaEnvironment;

    env["currentEngine"] = env;
    env[LUA_ENV_PTR_NAME] = this;

    env["valid"] = [&] (entt::entity e)
    {
        return entities.valid(e);
    };

    env["setName"] = [&] (entt::entity e, sol::optional<const char *> name)
    {
        return setName(e, name.has_value() ? name.value() : nullptr);
    };
    env["getName"] = [&] (entt::entity e) -> sol::optional<std::string>
    {
        if (Named *named = entities.try_get<Named>(e))
            return named->name_dont_change;
        else return sol::nullopt;
    };
    env["getByName"] = [&] (const char *name)
    {
        return getByName(name);
    };

    env["setComponent"] = [&] (entt::entity entity, const sol::table &component)
    {
        setComponentFromLua(entity, component);
    };

    env["setComponents"] = [&] (entt::entity entity, const sol::table &componentsTable)
    {
        for (const auto &[i, comp] : componentsTable)
            setComponentFromLua(entity, comp);
    };

    env["createEntity"] = [&] () -> entt::entity
    {
        return entities.create();
    };
    env["destroyEntity"] = [&] (entt::entity e)
    {
        entities.destroy(e);
    };
    env["createChild"] = [&] (entt::entity parentEntity, sol::optional<std::string> childName) -> entt::entity
    {
        return createChild(parentEntity, childName.value_or("").c_str());
    };
    env["getChild"] = [&] (entt::entity parentEntity, const char *childName) -> entt::entity
    {
        return getChildByName(parentEntity, childName);
    };
    env["applyTemplate"] = [&] (entt::entity extendE, const char *templateName, const sol::optional<sol::table> &extendArgs, sol::optional<bool> persistent)
    {
        auto entityTemplate = &getTemplate(templateName); // could throw error :)

        bool makePersistent = persistent.value_or(false);

        if (LuaTemplate *luaEntityTemplate = dynamic_cast<LuaTemplate *>(entityTemplate))
        {
            luaEntityTemplate->createComponentsWithLuaArguments(extendE, extendArgs, makePersistent);
        }
        else
            entityTemplate->createComponents(extendE, makePersistent);
    };

    env["onEntityEvent"] = [&] (entt::entity entity, const char *eventName, const sol::function &listener)
    {

        auto &emitter = entities.get_or_assign<EventEmitter>(entity);
        emitter.on(eventName, listener);
    };
    env["onEvent"] = [&] (const char *eventName, const sol::function &listener)
    {
        events.on(eventName, listener);
    };

    env["setTimeout"] = [&] (entt::entity e, float time, const sol::function &func)
    {
        auto &f = entities.get_or_assign<LuaScripted>(e).timeoutFuncs.emplace_back();
        f.timer = time;
        f.func = func;
    };

    sol::table componentTable = env["component"].get_or_create<sol::table>();
    for (auto &[name, info] : dibidab::getAllComponentInfos())
    {
        sol::table utilsTable = componentTable[name].template get_or_create<sol::table>();
        info.fillLuaUtilsTable(utilsTable, entities, &info);
    }
}

void dibidab::ecs::Engine::setParent(entt::entity child, entt::entity parent, const char *childName)
{
    Child c;
    c.parent = parent;
    c.name = childName;
    entities.assign<Child>(child, c);
}

entt::entity dibidab::ecs::Engine::createChild(entt::entity parent, const char *childName)
{
    entt::entity e = entities.create();
    setParent(e, parent, childName);
    return e;
}

void dibidab::ecs::Engine::update(double deltaTime)
{
    if (!bInitialized)
        throw gu_err("Cannot update non-initialized Engine!");

    bUpdating = true;

    for (auto sys : getSystemsToUpdate())
    {
        gu::profiler::Zone sysZone(sys->name);

        if (sys->updateFrequency == .0) sys->update(deltaTime, this);
        else
        {
            float customDeltaTime = 1.0f / sys->updateFrequency;
            sys->updateAccumulator += deltaTime;
            while (sys->updateAccumulator > customDeltaTime)
            {
                sys->update(customDeltaTime, this);
                sys->updateAccumulator -= customDeltaTime;
            }
        }
    }
    bUpdating = false;
}

bool dibidab::ecs::Engine::isUpdating() const
{
    return bUpdating;
}

void dibidab::ecs::Engine::onChildCreation(entt::registry &reg, entt::entity entity)
{
    Child &child = reg.get<Child>(entity);

    Parent &parent = reg.get_or_assign<Parent>(child.parent);

    parent.children.push_back(entity);
    if (!child.name.empty())
        parent.childrenByName[child.name] = entity;
}

void dibidab::ecs::Engine::onChildDeletion(entt::registry &reg, entt::entity entity)
{
    Child &child = reg.get<Child>(entity);
    Parent &parent = reg.get_or_assign<Parent>(child.parent);

    if (!child.name.empty())
        parent.childrenByName.erase(child.name);

    for (int i = 0; i < parent.children.size(); i++)
    {
        if (parent.children[i] != entity)
            continue;

        parent.children[i] = parent.children.back();
        parent.children.pop_back();
    }
}

void dibidab::ecs::Engine::onParentDeletion(entt::registry &reg, entt::entity entity)
{
    Parent &parent = reg.get_or_assign<Parent>(entity);

    auto tmpChildren(parent.children);
    parent.children.clear();

    if (parent.bDestroyChildrenOnDestroy)
        for (auto child : tmpChildren)
            reg.destroy(child);
    else
        for (auto child : tmpChildren)
            reg.remove<Child>(child);
}

entt::entity dibidab::ecs::Engine::getByName(const char *name) const
{
    auto it = namedEntities.find(name);
    if (it == namedEntities.end())
        return entt::null;
    return it->second;
}

bool dibidab::ecs::Engine::setName(entt::entity e, const char *name)
{
    if (name)
    {
        if (strcmp(name, "") == 0)
            throw gu_err("Tried to set name of entity#" + std::to_string(int(e)) + " to empty string! Pass nullptr or nil instead to remove the name.");

        auto claimedBy = getByName(name);
        if (claimedBy == e)
            return true;
        if (claimedBy == entt::null)
        {
            entities.remove_if_exists<Named>(e);
            entities.assign<Named>(e, name);
            namedEntities[name] = e;
            return true;
        }
        else return false;
    }
    else
    {
        entities.remove_if_exists<Named>(e);
        return true;
    }
}

void dibidab::ecs::Engine::onEntityDenaming(entt::registry &, entt::entity e)
{
    namedEntities.erase(entities.get<Named>(e).name_dont_change);
}

const char *dibidab::ecs::Engine::getName(entt::entity e) const
{
    if (const Named *named = entities.try_get<Named>(e))
        return named->name_dont_change.c_str();
    else return nullptr;
}

const std::unordered_map<std::string, entt::entity> &dibidab::ecs::Engine::getNamedEntities() const
{
    return namedEntities;
}

dibidab::ecs::Observer &dibidab::ecs::Engine::getObserverForComponent(const ComponentInfo &component)
{
    auto it = observerPerComponent.find(&component);
    if (it == observerPerComponent.end())
    {
        it = observerPerComponent.emplace(&component, component.createObserver(this->entities)).first;
    }
    return *it->second;
}

std::list<dibidab::ecs::System *> dibidab::ecs::Engine::getSystemsToUpdate() const
{
    std::list<System *> systemsToUpdate;
    for (System *sys : systems)
    {
        if (sys->bUpdatesEnabled)
        {
            systemsToUpdate.push_back(sys);
        }
    }
    return systemsToUpdate;
}

void dibidab::ecs::Engine::setComponentFromLua(entt::entity entity, const sol::table &component)
{
    if (component.get_type() != sol::type::userdata)
    {
        throw gu_err("Given object is not a registered type");
    }

    const char *structId = component["__type"]["name"].get<const char *>();

    if (const dibidab::StructInfo *structInfo = dibidab::findStructInfo(structId))
    {
        if (structInfo->componentInfo != nullptr)
        {
            structInfo->componentInfo->setFromLua(component, entity, entities);
        }
    }
}
