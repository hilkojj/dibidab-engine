
#include "EntityEngine.h"
#include <utils/hashing.h>
#include <utils/gu_error.h>
#include "./systems/AnimationSystem.h"
#include "entity_templates/LuaEntityTemplate.h"
#include "../generated/Children.hpp"
#include "../generated/Position3d.hpp"

void EntityEngine::addSystem(EntitySystem *sys, bool pushFront)
{
    assert(!initialized);
    if (pushFront)
        systems.push_front(sys);
    else
        systems.push_back(sys);
}

EntityTemplate &EntityEngine::getTemplate(std::string name)
{
    try {
        return getTemplate(hashStringCrossPlatform(name));
    }
    catch (_gu_err &)
    {
        throw gu_err("No EntityTemplate named " + name + " found");
    }
}

EntityTemplate &EntityEngine::getTemplate(int templateHash)
{
    auto t = entityTemplates[templateHash];
    if (!t)
        throw gu_err("No EntityTemplate found for hash " + std::to_string(templateHash));
    return *t;
}

const std::vector<std::string> &EntityEngine::getTemplateNames() const
{
    return entityTemplateNames;
}

entt::entity EntityEngine::getChildByName(entt::entity parent, const char *childName)
{
    const Parent *p = entities.try_get<Parent>(parent);
    if (!p)
        return entt::null;
    auto it = p->childrenByName.find(childName);
    if (it == p->childrenByName.end())
        return entt::null;
    return it->second;
}

void EntityEngine::registerLuaEntityTemplate(const char *assetPath)
{
    auto name = splitString(assetPath, "/").back();
    if (stringStartsWith(name, "_"))
        return;

    addEntityTemplate(name, new LuaEntityTemplate(assetPath, name.c_str(), this));
}

void EntityEngine::addEntityTemplate(const std::string &name, EntityTemplate *t)
{
    int hash = hashStringCrossPlatform(name);

    bool replace = entityTemplates[hash] != NULL;

    delete entityTemplates[hash];
    auto et = entityTemplates[hash] = t;
    et->engine = this;
    et->templateHash = hash;

    if (!replace)
        entityTemplateNames.push_back(name);
}

EntityEngine::~EntityEngine()
{
    events.emit(0, "BeforeDelete");

    for (auto sys : systems)
        delete sys;
    for (auto &entry : entityTemplates)
        delete entry.second;
}

struct Named {
    std::string name_dont_change;
};

void EntityEngine::initialize()
{
    assert(!initialized);

    addSystem(new AnimationSystem("animations"));

    entities.on_construct<Child>().connect<&EntityEngine::onChildCreation>(this);
    entities.on_destroy<Child>().connect<&EntityEngine::onChildDeletion>(this);

    entities.on_destroy<Parent>().connect<&EntityEngine::onParentDeletion>(this);

    entities.on_destroy<Named>().connect<&EntityEngine::onEnitiyDenaming>(this);

    initializeLuaEnvironment();

    for (auto &el : AssetManager::getLoadedAssetsForType<luau::Script>())
        if (stringStartsWith(el.first, "scripts/entities/"))
            registerLuaEntityTemplate(el.second->shortPath.c_str());

    for (auto sys : systems)
        sys->init(this);

    initialized = true;
}

void setComponentFromLua(entt::entity entity, const sol::table &component, entt::registry &reg)
{
    if (component.get_type() != sol::type::userdata)
        throw gu_err("Given object is not a registered type");

    auto typeName = component["__type"]["name"].get<std::string>();

    EntityEngine::componentUtils(typeName).setFromLuaTable(component, entt::entity(entity), reg);
}

void EntityEngine::initializeLuaEnvironment()
{
    // todo: functions might be called after EntityEngine is destructed

    luaEnvironment = sol::environment(luau::getLuaState(), sol::create, luau::getLuaState().globals());
    auto &env = luaEnvironment;

    env["currentEngine"] = env;

    env["valid"] = [&](entt::entity e) {
        return entities.valid(e);
    };

    env["setName"] = [&](entt::entity e, sol::optional<const char *> name) {
        return setName(e, name.has_value() ? name.value() : NULL);
    };
    env["getName"] = [&](entt::entity e) -> sol::optional<std::string> {
        if (Named *named = entities.try_get<Named>(e))
            return named->name_dont_change;
        else return sol::nullopt;
    };
    env["getByName"] = [&] (const char *name) {
        return getByName(name);
    };

    env["setComponent"] = [&](entt::entity entity, const sol::table &component) {
        setComponentFromLua(entity, component, entities);
    };

    env["setComponentFromJson"] = [&](entt::entity entity, const char *compName, const json &j) {
        componentUtils(compName).setJsonComponentWithKeys(j, entity, entities);
    };

    env["setComponents"] = [&](entt::entity entity, const sol::table &componentsTable) {

        for (const auto &[i, comp] : componentsTable)
            setComponentFromLua(entity, comp, entities);
    };

    env["createEntity"] = [&]() -> entt::entity {

        return entities.create();
    };
    env["destroyEntity"] = [&](entt::entity e) {

        entities.destroy(e);
    };
    env["createChild"] = [&](entt::entity parentEntity, sol::optional<std::string> childName) -> entt::entity {

        return createChild(parentEntity, childName.value_or("").c_str());
    };
    env["getChild"] = [&](entt::entity parentEntity, const char *childName) -> entt::entity {

        return getChildByName(parentEntity, childName);
    };
    env["applyTemplate"] = [&](entt::entity extendE, const char *templateName, const sol::optional<sol::table> &extendArgs, sol::optional<bool> persistent) {

        auto entityTemplate = &getTemplate(templateName); // could throw error :)

        bool makePersistent = persistent.value_or(false);

        if (dynamic_cast<LuaEntityTemplate *>(entityTemplate))
        {
            ((LuaEntityTemplate *) entityTemplate)->
                    createComponentsWithLuaArguments(extendE, extendArgs, makePersistent);
        }
        else
            entityTemplate->createComponents(extendE, makePersistent);
    };

    env["onEntityEvent"] = [&](entt::entity entity, const char *eventName, const sol::function &listener) {

        auto &emitter = entities.get_or_assign<EventEmitter>(entity);
        emitter.on(eventName, listener);
    };
    env["onEvent"] = [&](const char *eventName, const sol::function &listener) {
        events.on(eventName, listener);
    };

    auto componentUtilsTable = env["component"].get_or_create<sol::table>();
    for (auto &componentName : ComponentUtils::getAllComponentTypeNames())
        ComponentUtils::getFor(componentName)->registerLuaFunctions(componentUtilsTable, entities);
}

void EntityEngine::luaTableToComponent(entt::entity e, const std::string &componentName, const sol::table &component)
{
    componentUtils(componentName).setFromLuaTable(component, e, entities);
}

// todo: ComponentUtils::getFor() and ComponentUtils::tryGetFor()
const ComponentUtils &EntityEngine::componentUtils(const std::string &componentName)
{
    auto utils = ComponentUtils::getFor(componentName);
    if (!utils)
        throw gu_err("Component-type named '" + componentName + "' does not exist!");
    return *utils;
}

void EntityEngine::setParent(entt::entity child, entt::entity parent, const char *childName)
{
    Child c;
    c.parent = parent;
    c.name = childName;
    entities.assign<Child>(child, c);
}

entt::entity EntityEngine::createChild(entt::entity parent, const char *childName)
{
    entt::entity e = entities.create();
    setParent(e, parent, childName);
    return e;
}

void EntityEngine::update(double deltaTime)
{
    if (!initialized)
        throw gu_err("Cannot update non-initialized EntityEngine!");

    updating = true;

    for (auto sys : systems)
    {
        if (!sys->enabled) continue;
        gu::profiler::Zone sysZone(sys->name);

        if (sys->updateFrequency == .0) sys->update(deltaTime, this);
        else
        {
            float customDeltaTime = 1. / sys->updateFrequency;
            sys->updateAccumulator += deltaTime;
            while (sys->updateAccumulator > customDeltaTime)
            {
                sys->update(customDeltaTime, this);
                sys->updateAccumulator -= customDeltaTime;
            }
        }
    }
    updating = false;
}

void EntityEngine::onChildCreation(entt::registry &reg, entt::entity entity)
{
    Child &child = reg.get<Child>(entity);

    Parent &parent = reg.get_or_assign<Parent>(child.parent);

    parent.children.push_back(entity);
    if (!child.name.empty())
        parent.childrenByName[child.name] = entity;
}

void EntityEngine::onChildDeletion(entt::registry &reg, entt::entity entity)
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

void EntityEngine::onParentDeletion(entt::registry &reg, entt::entity entity)
{
    Parent &parent = reg.get_or_assign<Parent>(entity);

    auto tmpChildren(parent.children);
    parent.children.clear();

    if (parent.deleteChildrenOnDeletion)
        for (auto child : tmpChildren)
            reg.destroy(child);
    else
        for (auto child : tmpChildren)
            reg.remove<Child>(child);
}


vec3 EntityEngine::getPosition(entt::entity e) const
{
    return entities.has<Position3d>(e) ? entities.get<Position3d>(e).vec : vec3(0);
}

void EntityEngine::setPosition(entt::entity e, const vec3 &pos)
{
    entities.get_or_assign<Position3d>(e).vec = pos;
}

entt::entity EntityEngine::getByName(const char *name) const
{
    auto it = namedEntities.find(name);
    if (it == namedEntities.end())
        return entt::null;
    return it->second;
}

bool EntityEngine::setName(entt::entity e, const char *name)
{
    if (name)
    {
        if (strcmp(name, "") == 0)
            throw gu_err("Tried to set name of entity#" + std::to_string(int(e)) + " to empty string! Pass NULL or nil instead to remove the name.");

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

void EntityEngine::onEnitiyDenaming(entt::registry &, entt::entity e)
{
    namedEntities.erase(entities.get<Named>(e).name_dont_change);
}

const char *EntityEngine::getName(entt::entity e) const
{
    if (const Named *named = entities.try_get<Named>(e))
        return named->name_dont_change.c_str();
    else return NULL;
}


