
#include "Tree.h"

#include "ecs/systems/TimeOutSystem.h"
#include "level/room/Room.h"
#include "level/Level.h"

#include "utils/string_utils.h"

#include "imgui.h"

std::string getNodeErrorInfo(dibidab::behavior::Tree::Node *node)
{
    std::string info = node->getName();
    if (node->hasLuaDebugInfo())
    {
        const lua_Debug &debugInfo = node->getLuaDebugInfo();
        std::vector<std::string> pathSplitted = su::split(debugInfo.source, "/");
        if (!pathSplitted.empty())
        {
            info += "@" + pathSplitted.back() + ":" + std::to_string(debugInfo.currentline);
        }
    }
    return info;
}

dibidab::behavior::Tree::Node::Node() :
    parent(nullptr),
    bEntered(false),
    bAborted(false),
    luaDebugInfo(),
    bHasLuaDebugInfo(false)
{
    lua_State *luaState = luau::getLuaState().lua_state();
    if (lua_getstack(luaState, 1, &luaDebugInfo))
    {
        if (lua_getinfo(luaState, "nSl", &luaDebugInfo))
        {
            bHasLuaDebugInfo = true;
        }
    }
}

void dibidab::behavior::Tree::Node::enter()
{
    bEntered = true;
#ifndef NDEBUG
    if (parent && !parent->isEntered())
    {
        throw gu_err("Cannot enter a child whose parent was not entered!");
    }
#endif
}

void dibidab::behavior::Tree::Node::abort()
{
#ifndef NDEBUG
    if (!bEntered)
    {
        throw gu_err("Cannot abort a non-entered Node: " + getNodeErrorInfo(this));
    }
    if (bAborted)
    {
        throw gu_err("Cannot abort a Node again: " + getNodeErrorInfo(this));
    }
#endif
    bAborted = true;
}

void dibidab::behavior::Tree::Node::finish(Result result)
{
#ifndef NDEBUG
    if (!bEntered)
    {
        throw gu_err("Cannot finish a non-entered Node: " + getNodeErrorInfo(this));
    }
#endif
    if (bAborted)
    {
        if (result != Result::ABORTED)
        {
            throw gu_err("Cannot finish an aborted Node with an result other than ABORTED: " + getNodeErrorInfo(this));
        }
        bAborted = false;
    }
    bEntered = false;
#ifndef NDEBUG
    bFinishedAtLeastOnce = true;
    lastResult = result;
#endif
    if (parent != nullptr)
    {
        parent->onChildFinished(this, result);
    }
}

bool dibidab::behavior::Tree::Node::isEntered() const
{
    return bEntered;
}

bool dibidab::behavior::Tree::Node::isAborted() const
{
    return bAborted;
}

bool dibidab::behavior::Tree::Node::getEnteredDescription(std::vector<const char *> &descriptions) const
{
    if (!isEntered() || description.empty())
    {
        return false;
    }
    descriptions.push_back(description.c_str());
    return true;
}

dibidab::behavior::Tree::Node *dibidab::behavior::Tree::Node::setDescription(const char *inDescription)
{
    description = inDescription;
    return this;
}

bool dibidab::behavior::Tree::Node::hasLuaDebugInfo() const
{
    return bHasLuaDebugInfo;
}

const lua_Debug &dibidab::behavior::Tree::Node::getLuaDebugInfo() const
{
    return luaDebugInfo;
}

#ifndef NDEBUG
bool dibidab::behavior::Tree::Node::hasFinishedAtLeastOnce() const
{
    return bFinishedAtLeastOnce;
}

dibidab::behavior::Tree::Node::Result dibidab::behavior::Tree::Node::getLastResult() const
{
    return lastResult;
}
#endif

void dibidab::behavior::Tree::Node::registerAsParent(Node *child)
{
    if (child->parent != nullptr)
    {
        throw gu_err("Child already has a parent: " + getNodeErrorInfo(child));
    }
    child->parent = this;
}

void dibidab::behavior::Tree::CompositeNode::finish(Result result)
{
#ifndef NDEBUG
    for (Node *child : children)
    {
        if (child->isEntered())
        {
            throw gu_err(getNodeErrorInfo(this) + " wants to finish, but child " + getNodeErrorInfo(child) + " is entered!");
        }
    }
#endif
    Node::finish(result);
}

dibidab::behavior::Tree::CompositeNode *dibidab::behavior::Tree::CompositeNode::addChild(Node *child)
{
    if (child == nullptr)
    {
        throw gu_err("Child is null: " + getNodeErrorInfo(this));
    }
    registerAsParent(child);
    children.push_back(child);
    return this;
}

const std::vector<dibidab::behavior::Tree::Node *> &dibidab::behavior::Tree::CompositeNode::getChildren() const
{
    return children;
}

dibidab::behavior::Tree::CompositeNode::~CompositeNode()
{
    for (Node *child : children)
    {
        delete child;
    }
    children.clear();
}

bool dibidab::behavior::Tree::CompositeNode::getEnteredDescription(std::vector<const char *> &descriptions) const
{
    if (!isEntered())
    {
        return false;
    }
    bool bHasAnyChildDescription = false;
    for (const Node *child : children)
    {
        if (child->getEnteredDescription(descriptions))
        {
            bHasAnyChildDescription = true;
        }
    }
    if (!bHasAnyChildDescription)
    {
        return Node::getEnteredDescription(descriptions);
    }
    return bHasAnyChildDescription;
}

void dibidab::behavior::Tree::DecoratorNode::abort()
{
    Node::abort();
    if (child && child->isEntered())
    {
        child->abort();
    }
    else
    {
        finish(Node::Result::ABORTED);
    }
}

void dibidab::behavior::Tree::DecoratorNode::finish(Result result)
{
    if (child != nullptr && child->isEntered())
    {
        throw gu_err(getNodeErrorInfo(this) + " wants to finish, but child " + getNodeErrorInfo(child) + " is entered!");
    }
    Node::finish(result);
}

dibidab::behavior::Tree::DecoratorNode *dibidab::behavior::Tree::DecoratorNode::setChild(Node *inChild)
{
    if (child != nullptr)
    {
        if (isEntered())
        {
            throw gu_err("Cannot set child on a Decorator Node that is currently entered: " + getNodeErrorInfo(this));
        }
        delete child;
    }
    registerAsParent(inChild);
    child = inChild;
    return this;
}

dibidab::behavior::Tree::Node *dibidab::behavior::Tree::DecoratorNode::getChild() const
{
    return child;
}

bool dibidab::behavior::Tree::DecoratorNode::getEnteredDescription(std::vector<const char *> &descriptions) const
{
    if (!isEntered())
    {
        return false;
    }
    if (child != nullptr && child->getEnteredDescription(descriptions))
    {
        return true;
    }
    return Node::getEnteredDescription(descriptions);
}

dibidab::behavior::Tree::DecoratorNode::~DecoratorNode()
{
    delete child;
}

constexpr static int INVALID_CHILD_INDEX = -1;

dibidab::behavior::Tree::SequenceNode::SequenceNode() :
    currentChildIndex(INVALID_CHILD_INDEX)
{
}

void dibidab::behavior::Tree::SequenceNode::enter()
{
    if (currentChildIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Already entered this SequenceNode: " + getNodeErrorInfo(this));
    }
    Tree::Node::enter();

    if (getChildren().empty())
    {
        finish(Node::Result::SUCCESS);
    }
    else
    {
        currentChildIndex = 0;
        getChildren()[currentChildIndex]->enter();
    }
}

void dibidab::behavior::Tree::SequenceNode::abort()
{
    Tree::Node::abort();
#ifndef NDEBUG
    if (currentChildIndex == INVALID_CHILD_INDEX)
    {
        throw gu_err("Trying to abort a sequence that has not entered a child: " + getNodeErrorInfo(this));
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void dibidab::behavior::Tree::SequenceNode::finish(Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    Tree::CompositeNode::finish(result);
}

const char *dibidab::behavior::Tree::SequenceNode::getName() const
{
    return "Sequence";
}

void checkCorrectChildFinished(dibidab::behavior::Tree::Node *parent, const std::vector<dibidab::behavior::Tree::Node *> &children,
    int currentChildIndex, dibidab::behavior::Tree::Node *finishedChild)
{
    if (currentChildIndex == INVALID_CHILD_INDEX || !parent->isEntered())
    {
        if (!parent->isEntered())
        {
            throw gu_err("Child (" + getNodeErrorInfo(finishedChild) + ") finished, but parent (" + getNodeErrorInfo(parent) + ") was not entered!");
        }
        else
        {
            throw gu_err("Child (" + getNodeErrorInfo(finishedChild) + ") finished, but parent (" + getNodeErrorInfo(parent) + ") thinks no child was entered!");
        }
    }
    if (finishedChild != children[currentChildIndex])
    {
        throw gu_err("Child that finished is not the current child that was entered!\nFinished child: " + getNodeErrorInfo(finishedChild)
            + "\nEntered child: " + getNodeErrorInfo(children[currentChildIndex]) + "\nParent: " + getNodeErrorInfo(parent));
    }

}

void dibidab::behavior::Tree::SequenceNode::onChildFinished(Node *child, Result result)
{
    Tree::CompositeNode::onChildFinished(child, result);

    const std::vector<Node *> &children = getChildren();

    checkCorrectChildFinished(this, children, currentChildIndex, child);

    if (result == Node::Result::ABORTED)
    {
        finish(Node::Result::ABORTED);
    }
    else if (result == Node::Result::FAILURE)
    {
        finish(Node::Result::FAILURE);
    }
    else if (++currentChildIndex == children.size())
    {
        finish(Node::Result::SUCCESS);
    }
    else
    {
        children[currentChildIndex]->enter();
    }
}

dibidab::behavior::Tree::SelectorNode::SelectorNode() :
    currentChildIndex(INVALID_CHILD_INDEX)
{
}

void dibidab::behavior::Tree::SelectorNode::enter()
{
    if (currentChildIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Already entered this SelectorNode: " + getNodeErrorInfo(this));
    }
    Tree::Node::enter();

    if (getChildren().empty())
    {
        // TODO
        finish(Node::Result::SUCCESS);
    }
    else
    {
        currentChildIndex = 0;
        getChildren()[currentChildIndex]->enter();
    }
}

void dibidab::behavior::Tree::SelectorNode::abort()
{
    Tree::Node::abort();
#ifndef NDEBUG
    if (currentChildIndex == INVALID_CHILD_INDEX)
    {
        throw gu_err("Trying to abort a selector that has not entered a child: " + getNodeErrorInfo(this));
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void dibidab::behavior::Tree::SelectorNode::finish(Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    Tree::CompositeNode::finish(result);
}

const char *dibidab::behavior::Tree::SelectorNode::getName() const
{
    return "Selector";
}

void dibidab::behavior::Tree::SelectorNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);

    const std::vector<Node *> &children = getChildren();

    checkCorrectChildFinished(this, children, currentChildIndex, child);

    if (result == Node::Result::ABORTED)
    {
        finish(Node::Result::ABORTED);
    }
    else if (result == Node::Result::SUCCESS)
    {
        finish(Node::Result::SUCCESS);
    }
    else if (++currentChildIndex == children.size())
    {
        finish(Node::Result::FAILURE);
    }
    else
    {
        children[currentChildIndex]->enter();
    }
}

dibidab::behavior::Tree::ParallelNode::ParallelNode() :
    numChildrenFinished(0)
{

}

void dibidab::behavior::Tree::ParallelNode::enter()
{
    CompositeNode::enter();
    if (getChildren().empty())
    {
        finish(Node::Result::SUCCESS);
        return;
    }
    numChildrenFinished = 0;
    for (Node *child : getChildren())
    {
        child->enter();
    }
}

void dibidab::behavior::Tree::ParallelNode::abort()
{
    CompositeNode::abort();
    // Abort all entered children.
    for (Node *child : getChildren())
    {
        if (child->isEntered())
        {
            child->abort();
        }
    }
}

const char *dibidab::behavior::Tree::ParallelNode::getName() const
{
    return "Parallel";
}

void dibidab::behavior::Tree::ParallelNode::onChildFinished(Node *child, Node::Result result)
{
    // Finish if all children are finished.
    if (++numChildrenFinished == getChildren().size())
    {
        CompositeNode::finish(isAborted() ? Node::Result::ABORTED : Node::Result::SUCCESS);
    }
}

void dibidab::behavior::Tree::InverterNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("InverterNode has no child: " + getNodeErrorInfo(this));
    }
    Tree::Node::enter();
    child->enter();
}

const char *dibidab::behavior::Tree::InverterNode::getName() const
{
    return "Inverter";
}

void dibidab::behavior::Tree::InverterNode::onChildFinished(Node *child, Result result)
{
    if (result == Result::ABORTED)
    {
        finish(Result::ABORTED);
        return;
    }
    finish(result == Result::SUCCESS ? Result::FAILURE : Result::SUCCESS);
}

void dibidab::behavior::Tree::SucceederNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("SucceederNode has no child: " + getNodeErrorInfo(this));
    }
    Node::enter();
    child->enter();
}

const char *dibidab::behavior::Tree::SucceederNode::getName() const
{
    return "Succeeder";
}

void dibidab::behavior::Tree::SucceederNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    if (result == Result::ABORTED)
    {
        finish(Result::ABORTED);
        return;
    }
    finish(Result::SUCCESS);
}

void dibidab::behavior::Tree::RepeaterNode::enter()
{
#ifndef NDEBUG
    timesRepeated = 0;
#endif
    Node::enter();
    enterChild();
}

void dibidab::behavior::Tree::RepeaterNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    switch (result)
    {
        case Result::SUCCESS:
#ifndef NDEBUG
            timesRepeated++;
#endif
            enterChild();
            break;
        case Result::FAILURE:
            finish(Result::SUCCESS);
            break;
        case Result::ABORTED:
            finish(Result::ABORTED);
            break;
    }
}

void dibidab::behavior::Tree::RepeaterNode::enterChild()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("RepeaterNode has no child: " + getNodeErrorInfo(this));
    }
    child->enter();
}

const char *dibidab::behavior::Tree::RepeaterNode::getName() const
{
    return "Repeater";
}

void dibidab::behavior::Tree::RepeaterNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
#ifndef NDEBUG
    ImGui::Text("%dx", timesRepeated);
#endif
}

void dibidab::behavior::Tree::ComponentDecoratorNode::addWhileEntered(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getNodeErrorInfo(this));
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getNodeErrorInfo(this));
    }
    toAddWhileEntered.push_back({engine, entity, component});
}

void dibidab::behavior::Tree::ComponentDecoratorNode::addOnEnter(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getNodeErrorInfo(this));
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getNodeErrorInfo(this));
    }
    toAddOnEnter.push_back({engine, entity, component});
}

void dibidab::behavior::Tree::ComponentDecoratorNode::removeOnFinish(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getNodeErrorInfo(this));
    }
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr! " + getNodeErrorInfo(this));
    }
    toRemoveOnFinish.push_back({engine, entity, component});
}

void dibidab::behavior::Tree::ComponentDecoratorNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("ComponentDecoratorNode has no child: " + getNodeErrorInfo(this));
    }
    Node::enter();

    for (const std::vector<EntityComponent> &toAddVector : { toAddWhileEntered, toAddOnEnter })
    {
        for (const EntityComponent entityComponent : toAddVector)
        {
            if (!entityComponent.engine->entities.valid(entityComponent.entity))
            {
                throw gu_err("Cannot add " + std::string(entityComponent.component->name) +
                    " to an invalid entity while entering: " + getNodeErrorInfo(this));
            }
            entityComponent.component->addComponent(entityComponent.entity, entityComponent.engine->entities);
        }
    }

    child->enter();
}

void dibidab::behavior::Tree::ComponentDecoratorNode::finish(Result result)
{
    for (const std::vector<EntityComponent> &entityComponentsToRemove : { toAddWhileEntered, toRemoveOnFinish })
    {
        for (const EntityComponent entityComponent : entityComponentsToRemove)
        {
            if (!entityComponent.engine->entities.valid(entityComponent.entity))
            {
                continue; // Ignore.
            }
            entityComponent.component->removeComponent(entityComponent.entity, entityComponent.engine->entities);
        }
    }
    DecoratorNode::finish(result);
}

const char *dibidab::behavior::Tree::ComponentDecoratorNode::getName() const
{
    return "ComponentDecorator";
}

void dibidab::behavior::Tree::ComponentDecoratorNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
    for (int i = 0; i < toAddWhileEntered.size(); i++)
    {
        if (i > 0)
        {
            ImGui::TextDisabled(" | ");
            ImGui::SameLine();
        }
        const EntityComponent &entityComponent = toAddWhileEntered[i];

        ImGui::TextDisabled("Add ");
        ImGui::SameLine();
        ImGui::Text("%s", entityComponent.component->name);
        ImGui::SameLine();
        ImGui::TextDisabled(" to ");
        ImGui::SameLine();

        if (!entityComponent.engine->entities.valid(entityComponent.entity))
        {
            ImGui::Text("Invalid entity #%d", int(entityComponent.entity));
            ImGui::SameLine();
        }
        else if (const char *entityName = entityComponent.engine->getName(entityComponent.entity))
        {
            ImGui::Text("#%d %s", int(entityComponent.entity), entityName);
            ImGui::SameLine();
        }
        else
        {
            ImGui::Text("#%d", int(entityComponent.entity));
            ImGui::SameLine();
        }
    }
}

void dibidab::behavior::Tree::ComponentDecoratorNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    finish(result);
}

dibidab::behavior::Tree::WaitNode::WaitNode() :
    seconds(-1.0f),
    waitingEntity(entt::null),
    engine(nullptr)
#ifndef NDEBUG
    ,
    timeStarted(0.0f)
#endif
{}

dibidab::behavior::Tree::WaitNode *dibidab::behavior::Tree::WaitNode::finishAfter(const float inSeconds, const entt::entity inWaitingEntity,
    ecs::Engine *inEngine)
{
    seconds = inSeconds;
    waitingEntity = inWaitingEntity;
    engine = inEngine;
#ifndef NDEBUG
    if (inEngine == nullptr)
    {
        throw gu_err("Engine is a nullptr!\n" + getNodeErrorInfo(this));
    }
    if (!inEngine->entities.valid(inWaitingEntity))
    {
        throw gu_err("Entity is not valid!\n" + getNodeErrorInfo(this));
    }
#endif
    return this;
}

void dibidab::behavior::Tree::WaitNode::enter()
{
    Node::enter();
    if (seconds >= 0.0f && engine != nullptr)
    {
        if (!engine->entities.valid(waitingEntity))
        {
            throw gu_err("Entity #" + std::to_string(int(waitingEntity)) + " is not valid!\n" + getNodeErrorInfo(this));
        }
        onWaitFinished = engine->getTimeOuts()->unsafeCallAfter(seconds, waitingEntity, [&]()
        {
            finish(Node::Result::SUCCESS);
        });
#ifndef NDEBUG
        if (level::Room *room = dynamic_cast<level::Room *>(engine))
        {
            timeStarted = float(room->getLevel().getTime());
        }
#endif
    }
}

void dibidab::behavior::Tree::WaitNode::abort()
{
    Node::abort();
    finish(Node::Result::ABORTED);
}

void dibidab::behavior::Tree::WaitNode::finish(Node::Result result)
{
    onWaitFinished.reset();
    Node::finish(result);
}

const char *dibidab::behavior::Tree::WaitNode::getName() const
{
    return "Wait";
}

void dibidab::behavior::Tree::WaitNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
    const bool bUsingTime = seconds >= 0.0f && engine != nullptr;
    if (bUsingTime)
    {
        ImGui::Text("%.2fs", seconds);
    }
    else
    {
        ImGui::TextDisabled("Until abort");
    }
    if (ImGui::IsItemHovered())
    {
        if (engine != nullptr && engine->entities.valid(waitingEntity))
        {
            if (const char *waitingEntityName = engine->getName(waitingEntity))
            {
                ImGui::SetTooltip("Entity: #%d %s", int(waitingEntity), waitingEntityName);
            }
            else
            {
                ImGui::SetTooltip("Entity: #%d", int(waitingEntity));
            }
        }
    }
#ifndef NDEBUG
    if (bUsingTime && isEntered())
    {
        if (level::Room *room = dynamic_cast<level::Room *>(engine))
        {
            ImGui::SameLine();
            const float timeElapsed = float(room->getLevel().getTime()) - timeStarted;
            ImGui::ProgressBar(timeElapsed / seconds, ImVec2(100.0f, 0.0f), std::string(std::to_string(int(timeElapsed))
                + "." + std::to_string(int(fract(timeElapsed) * 10.0f)) + "s").c_str());
        }
    }
#endif

}

dibidab::behavior::Tree::ComponentObserverNode::ComponentObserverNode() :
    bUseSafetyDelay(true),
    fulfilledNodeIndex(INVALID_CHILD_INDEX),
    unfulfilledNodeIndex(INVALID_CHILD_INDEX),
    bFulFilled(false),
    currentNodeIndex(INVALID_CHILD_INDEX)
{
}

void dibidab::behavior::Tree::ComponentObserverNode::enter()
{
    CompositeNode::enter();
    enterChild();
}

void dibidab::behavior::Tree::ComponentObserverNode::abort()
{
    CompositeNode::abort();
    if (currentNodeIndex != INVALID_CHILD_INDEX)
    {
        getChildren().at(currentNodeIndex)->abort();
    }
    else
    {
        finish(Node::Result::ABORTED);
    }
}

void dibidab::behavior::Tree::ComponentObserverNode::finish(Result result)
{
    currentNodeIndex = INVALID_CHILD_INDEX;
    Node::finish(result);
}

dibidab::behavior::Tree::ComponentObserverNode *dibidab::behavior::Tree::ComponentObserverNode::withoutSafetyDelay()
{
    if (!observerHandles.empty())
    {
        throw gu_err("Safety delay can only be disabled before observers are added: " + getNodeErrorInfo(this));
    }
    bUseSafetyDelay = false;
    return this;
}

void dibidab::behavior::Tree::ComponentObserverNode::has(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    observe(engine, entity, component, true, false);
}

void dibidab::behavior::Tree::ComponentObserverNode::exclude(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    observe(engine, entity, component, false, true);
}

dibidab::behavior::Tree::ComponentObserverNode *dibidab::behavior::Tree::ComponentObserverNode::setOnFulfilledNode(Node *child)
{
    if (fulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set fulfilled child for a second time: " + getNodeErrorInfo(this));
    }
    fulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

dibidab::behavior::Tree::ComponentObserverNode *dibidab::behavior::Tree::ComponentObserverNode::setOnUnfulfilledNode(Node *child)
{
    if (unfulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set unfulfilled child for a second time: " + getNodeErrorInfo(this));
    }
    unfulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

dibidab::behavior::Tree::CompositeNode *dibidab::behavior::Tree::ComponentObserverNode::addChild(Node *child)
{
    throw gu_err("Do not call addChild on this node: " + getNodeErrorInfo(this));
}

const char *dibidab::behavior::Tree::ComponentObserverNode::getName() const
{
    return "ComponentObserver";
}

void dibidab::behavior::Tree::ComponentObserverNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
    // TODO/WARNING: this debug info relies heavily on the implementation details.

    for (int i = 0; i < observerHandles.size() / 2; i++)
    {
        if (i > 0)
        {
            ImGui::TextDisabled(" | ");
            ImGui::SameLine();
        }
        const ObserverHandle& observerHandle = observerHandles[i * 2];
        const entt::entity entity = observerHandle.handle.getEntity();

        if (!observerHandle.engine->entities.valid(entity))
        {
            ImGui::Text("Invalid entity #%d", int(entity));
            continue;
        }
        const bool bCurrentValue = conditions[i];

        bool bTmpValue = bCurrentValue;
        ImGui::Checkbox("", &bTmpValue);
        ImGui::SameLine();

        const bool bHasComponent = observerHandle.component->hasComponent(entity,
            observerHandle.engine->entities);

        if (const char *entityName = observerHandle.engine->getName(entity))
        {
            ImGui::Text("#%d %s ", int(entity), entityName);
            ImGui::SameLine();
        }
        ImGui::TextDisabled(bCurrentValue == bHasComponent ? "has " : "excludes ");
        ImGui::SameLine();
        ImGui::Text("%s", observerHandle.component->name);
        ImGui::SameLine();
    }
}

void dibidab::behavior::Tree::ComponentObserverNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    if (result == Node::Result::ABORTED)
    {
        if (isAborted())
        {
            // Observer was aborted.
            finish(Node::Result::ABORTED);
            return;
        }
        else if (getChildIndexToEnter() != currentNodeIndex)
        {
            enterChild();
            return;
        }
    }
    else
    {
        checkCorrectChildFinished(this, getChildren(), currentNodeIndex, child);
        finish(result);
    }
}

void dibidab::behavior::Tree::ComponentObserverNode::observe(ecs::Engine *engine, entt::entity entity,
    const dibidab::ComponentInfo *component, const bool presentValue, const bool absentValue)
{
    if (isEntered())
    {
        throw gu_err("Cannot edit while entered: " + getNodeErrorInfo(this));
    }
#ifndef NDEBUG
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr: " + getNodeErrorInfo(this));
    }
    if (engine == nullptr)
    {
        throw gu_err("No Engine was given: " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Invalid entity was given: " + getNodeErrorInfo(this));
    }
#endif
    // TODO: Only register callbacks on enter. Remove callbacks when leaving or in destructor
    ecs::Observer &observer = engine->getObserverForComponent(*component);
    const int conditionIndex = int(conditions.size());

    std::function<void()> onConstructCallback = [this, engine, entity, conditionIndex, presentValue]
    {
        conditions[conditionIndex] = presentValue;
        onConditionsChanged(engine, entity);
    };

    ecs::Observer::Handle onConstructHandle = observer.onConstruct(entity,
        bUseSafetyDelay ? [this, engine, onConstructCallback, conditionIndex]
        {
            observerHandles[conditionIndex * 2ul].latestConditionChangedDelay =
                engine->getTimeOuts()->nextUpdate += onConstructCallback;
        }
        :
        onConstructCallback
    );

    std::function<void()> onDestroyCallback = [this, engine, entity, conditionIndex, absentValue]
    {
        conditions[conditionIndex] = absentValue;
        onConditionsChanged(engine, entity);
    };

    ecs::Observer::Handle onDestroyHandle = observer.onDestroy(entity,
        bUseSafetyDelay ? [this, engine, onDestroyCallback, conditionIndex]
        {
            observerHandles[conditionIndex * 2ul + 1ul].latestConditionChangedDelay =
                engine->getTimeOuts()->nextUpdate += onDestroyCallback;
        }
        :
        onDestroyCallback
    );

    observerHandles.push_back({
        engine, component, onConstructHandle
    });
    observerHandles.push_back({
        engine, component, onDestroyHandle
    });
    conditions.push_back(component->hasComponent(entity, engine->entities) ? presentValue : absentValue);
    bFulFilled = allConditionsFulfilled();
}

bool dibidab::behavior::Tree::ComponentObserverNode::allConditionsFulfilled() const
{
    for (const bool condition : conditions)
    {
        if (!condition)
        {
            return false;
        }
    }
    return true;
}
void dibidab::behavior::Tree::ComponentObserverNode::onConditionsChanged(ecs::Engine *engine, entt::entity entity)
{
    bool bNewFulfilled = allConditionsFulfilled();

    if (bNewFulfilled != bFulFilled)
    {
        bFulFilled = bNewFulfilled;

        if (isEntered())
        {
            if (currentNodeIndex != INVALID_CHILD_INDEX)
            {
                Node *toAbort = getChildren().at(currentNodeIndex);
                if (!toAbort->isAborted())
                {
                    toAbort->abort();
                }
            }
            else
            {
                enterChild();
            }
        }
    }
}

int dibidab::behavior::Tree::ComponentObserverNode::getChildIndexToEnter() const
{
    return bFulFilled ? fulfilledNodeIndex : unfulfilledNodeIndex;
}

void dibidab::behavior::Tree::ComponentObserverNode::enterChild()
{
    int toEnterIndex = getChildIndexToEnter();
    if (toEnterIndex == INVALID_CHILD_INDEX)
    {
        finish(Node::Result::SUCCESS);
    }
    else
    {
        currentNodeIndex = toEnterIndex;
        getChildren().at(toEnterIndex)->enter();
    }
}

dibidab::behavior::Tree::ComponentObserverNode::~ComponentObserverNode()
{
    for (ObserverHandle &observerHandle : observerHandles)
    {
        if (observerHandle.engine->isDestructing())
        {
            // getEntityObserver() will crash because the registry's context variables are destroyed already.
            // Unregistering is not needed because the callbacks will be destroyed anyway.
            continue;
        }
        observerHandle.engine->getObserverForComponent(*observerHandle.component).unregister(observerHandle.handle);
    }
}

dibidab::behavior::Tree::LuaLeafNode::LuaLeafNode() :
    bInEnterFunction(false)
{}

void dibidab::behavior::Tree::LuaLeafNode::enter()
{
    Node::enter();
    if (luaEnterFunction.valid())
    {
        bInEnterFunction = true;
        sol::protected_function_result result = luaEnterFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::enter failed for: " << getNodeErrorInfo(this) << "\n" << result.get<sol::error>().what() << std::endl;

            if (isEntered())
            {
                // note: will be ignored in case we're aborted and because bInEnterFunction is still true!
                finish(Node::Result::FAILURE);
            }
        }

        bInEnterFunction = false;

        if (isAborted())
        {
            finishAborted();
        }
    }
    else
    {
        std::cerr << "LuaLeafNode::enter failed because no enter function was set! " << getNodeErrorInfo(this) << std::endl;
        finish(Node::Result::FAILURE);
    }
}

void dibidab::behavior::Tree::LuaLeafNode::abort()
{
    Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void dibidab::behavior::Tree::LuaLeafNode::finish(Result result)
{
    if (bInEnterFunction && isAborted())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *dibidab::behavior::Tree::LuaLeafNode::getName() const
{
    return "LuaLeaf";
}

void dibidab::behavior::Tree::LuaLeafNode::finishAborted()
{
    if (luaAbortFunction.valid())
    {
        sol::protected_function_result result = luaAbortFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::abort failed for: " << getNodeErrorInfo(this) << "\n" << result.get<sol::error>().what() << std::endl;

            if (isAborted())
            {
                finish(Node::Result::ABORTED);
            }
        }
    }
    else
    {
        finish(Node::Result::ABORTED);
    }
}

dibidab::behavior::Tree::FunctionalLeafNode::FunctionalLeafNode() :
    bInEnterFunction(false)
{}

dibidab::behavior::Tree::FunctionalLeafNode *dibidab::behavior::Tree::FunctionalLeafNode::setOnEnter(
    std::function<void(FunctionalLeafNode &)> function
)
{
    onEnter = std::move(function);
    return this;
}

dibidab::behavior::Tree::FunctionalLeafNode *dibidab::behavior::Tree::FunctionalLeafNode::setOnAbort(
    std::function<void(FunctionalLeafNode &)> function
)
{
    onAbort = std::move(function);
    return this;
}

void dibidab::behavior::Tree::FunctionalLeafNode::enter()
{
    Node::enter();
    if (onEnter)
    {
        bInEnterFunction = true;
        onEnter(*this);
        bInEnterFunction = false;
        if (isAborted())
        {
            finishAborted();
        }
    }
    else
    {
        finish(Result::SUCCESS);
    }
}

void dibidab::behavior::Tree::FunctionalLeafNode::abort()
{
    Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void dibidab::behavior::Tree::FunctionalLeafNode::finish(Result result)
{
    if (bInEnterFunction && isAborted())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *dibidab::behavior::Tree::FunctionalLeafNode::getName() const
{
    return "FunctionalLeaf";
}

void dibidab::behavior::Tree::FunctionalLeafNode::finishAborted()
{
    if (onAbort)
    {
        onAbort(*this);
    }
    else
    {
        finish(Result::ABORTED);
    }
}

dibidab::behavior::Tree::Tree() :
    rootNode(nullptr)
{

}

void dibidab::behavior::Tree::setRootNode(Node *root)
{
    if (rootNode)
    {
        throw gu_err("A rootNode was already set!");
    }
    rootNode = std::shared_ptr<Node>(root);
}

dibidab::behavior::Tree::Node *dibidab::behavior::Tree::getRootNode() const
{
    return rootNode.get();
}

void dibidab::behavior::Tree::addToLuaEnvironment(sol::state *lua)
{
    lua->new_enum(
        "BTResult",
        "SUCCESS", Node::Result::SUCCESS,
        "FAILURE", Node::Result::FAILURE,
        "ABORTED", Node::Result::ABORTED
    );

    // ------------------------ Abstract Node classes: -------------------------- //

    sol::usertype<Node> nodeType = lua->new_usertype<Node>(
        "BTNode",

        "finish", &Node::finish,
        "setDescription", &Node::setDescription
    );

    sol::usertype<CompositeNode> compositeNodeType = lua->new_usertype<CompositeNode>(
        "BTCompositeNode",
        sol::base_classes,
        sol::bases<Node>(),

        "addChild", &CompositeNode::addChild
    );

    sol::usertype<DecoratorNode> decoratorNodeType = lua->new_usertype<DecoratorNode>(
        "BTDecoratorNode",
        sol::base_classes,
        sol::bases<Node>(),

        "setChild", &DecoratorNode::setChild
    );

    // ------------------------ Basic Node classes: -------------------------- //

    sol::usertype<SequenceNode> sequenceNodeType = lua->new_usertype<SequenceNode>(
        "BTSequenceNode",
        sol::factories([] ()
        {
            return new SequenceNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    sol::usertype<SelectorNode> selectorNodeType = lua->new_usertype<SelectorNode>(
        "BTSelectorNode",
        sol::factories([] ()
        {
            return new SelectorNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    sol::usertype<ParallelNode> parallelNodeType = lua->new_usertype<ParallelNode>(
        "BTParallelNode",
        sol::factories([] ()
        {
            return new ParallelNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    sol::usertype<InverterNode> inverterNodeType = lua->new_usertype<InverterNode>(
        "BTInverterNode",
        sol::factories([] ()
        {
            return new InverterNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    sol::usertype<SucceederNode> succeederNodeType = lua->new_usertype<SucceederNode>(
        "BTSucceederNode",
        sol::factories([] ()
        {
            return new SucceederNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    sol::usertype<RepeaterNode> repeaterNodeType = lua->new_usertype<RepeaterNode>(
        "BTRepeaterNode",
        sol::factories([] ()
        {
            return new RepeaterNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    sol::usertype<ComponentDecoratorNode> componentDecoratorNodeType = lua->new_usertype<ComponentDecoratorNode>(
        "BTComponentDecoratorNode",
        sol::factories([] ()
        {
            return new ComponentDecoratorNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>(),

        "addWhileEntered", [] (ComponentDecoratorNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
        -> ComponentDecoratorNode &
        {
            const dibidab::ComponentInfo *component = dibidab::getInfoFromUtilsTable(componentTable);
            node.addWhileEntered(currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME), entity, component);
            return node;
        },
        "addOnEnter", [] (ComponentDecoratorNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
        -> ComponentDecoratorNode &
        {
            const dibidab::ComponentInfo *component = dibidab::getInfoFromUtilsTable(componentTable);
            node.addOnEnter(currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME), entity, component);
            return node;
        },
        "removeOnFinish", [] (ComponentDecoratorNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
        -> ComponentDecoratorNode &
        {
            const dibidab::ComponentInfo *component = dibidab::getInfoFromUtilsTable(componentTable);
            node.removeOnFinish(currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME), entity, component);
            return node;
        }
    );

    sol::usertype<WaitNode> waitNodeType = lua->new_usertype<WaitNode>(
        "BTWaitNode",
        sol::factories([] ()
        {
            return new WaitNode();
        }),
        sol::base_classes,
        sol::bases<Node, LeafNode>(),

        "finishAfter", [] (WaitNode &node, float seconds, entt::entity waitingEntity,
            const sol::this_environment &currentEnv)
            -> WaitNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            node.finishAfter(seconds, waitingEntity, currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME));
            return node;
        }
    );

    // ------------------------ Event-based Node classes: -------------------------- //

    sol::usertype<ComponentObserverNode> componentObserverNodeType = lua->new_usertype<ComponentObserverNode>(
        "BTComponentObserverNode",
        sol::factories([] ()
        {
            return new ComponentObserverNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>(),

        "withoutSafetyDelay", &ComponentObserverNode::withoutSafetyDelay,

        "has", [] (ComponentObserverNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
            -> ComponentObserverNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            const dibidab::ComponentInfo *component = dibidab::getInfoFromUtilsTable(componentTable);
            node.has(currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME), entity, component);
            return node;
        },
        "exclude", [] (ComponentObserverNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
            -> ComponentObserverNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            const dibidab::ComponentInfo *component = dibidab::getInfoFromUtilsTable(componentTable);
            node.exclude(currentEnv.env.value().get<ecs::Engine *>(ecs::Engine::LUA_ENV_PTR_NAME), entity, component);
            return node;
        },
        "fulfilled", &ComponentObserverNode::setOnFulfilledNode,
        "unfulfilled", &ComponentObserverNode::setOnUnfulfilledNode
    );

    // ------------------------ Customization Node classes: -------------------------- //

    sol::usertype<LuaLeafNode> luaLeafNodeType = lua->new_usertype<LuaLeafNode>(
        "BTLuaLeafNode",
        sol::factories([] ()
        {
            return new LuaLeafNode();
        }),
        sol::base_classes,
        sol::bases<Node, LeafNode>(),

        "setEnterFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaEnterFunction = function;
            return luaLeafNode;
        },
        "setAbortFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaAbortFunction = function;
            return luaLeafNode;
        }
    );
}
