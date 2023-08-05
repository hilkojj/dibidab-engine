
#include "BehaviorTree.h"

#include "../../ecs/systems/TimeOutSystem.h"
#include "../../level/room/Room.h"
#include "../../level/Level.h"

#include <imgui.h>

std::string getNodeErrorInfo(BehaviorTree::Node *node)
{
    std::string info = node->getName();
    if (node->hasLuaDebugInfo())
    {
        const lua_Debug &debugInfo = node->getLuaDebugInfo();
        std::vector<std::string> pathSplitted = splitString(debugInfo.source, "/");
        if (!pathSplitted.empty())
        {
            info += "@" + pathSplitted.back() + ":" + std::to_string(debugInfo.currentline);
        }
    }
    return info;
}

BehaviorTree::Node::Node() :
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

void BehaviorTree::Node::enter()
{
    bEntered = true;
#ifndef NDEBUG
    if (parent && !parent->isEntered())
    {
        throw gu_err("Cannot enter a child whose parent was not entered!");
    }
#endif
}

void BehaviorTree::Node::abort()
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

void BehaviorTree::Node::finish(BehaviorTree::Node::Result result)
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

bool BehaviorTree::Node::isEntered() const
{
    return bEntered;
}

bool BehaviorTree::Node::isAborted() const
{
    return bAborted;
}

bool BehaviorTree::Node::hasLuaDebugInfo() const
{
    return bHasLuaDebugInfo;
}

const lua_Debug &BehaviorTree::Node::getLuaDebugInfo() const
{
    return luaDebugInfo;
}

#ifndef NDEBUG
bool BehaviorTree::Node::hasFinishedAtLeastOnce() const
{
    return bFinishedAtLeastOnce;
}

BehaviorTree::Node::Result BehaviorTree::Node::getLastResult() const
{
    return lastResult;
}
#endif

void BehaviorTree::Node::registerAsParent(BehaviorTree::Node *child)
{
    if (child->parent != nullptr)
    {
        throw gu_err("Child already has a parent: " + getNodeErrorInfo(child));
    }
    child->parent = this;
}

void BehaviorTree::CompositeNode::finish(BehaviorTree::Node::Result result)
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

BehaviorTree::CompositeNode *BehaviorTree::CompositeNode::addChild(BehaviorTree::Node *child)
{
    if (child == nullptr)
    {
        throw gu_err("Child is null: " + getNodeErrorInfo(this));
    }
    registerAsParent(child);
    children.push_back(child);
    return this;
}

const std::vector<BehaviorTree::Node *> &BehaviorTree::CompositeNode::getChildren() const
{
    return children;
}

BehaviorTree::CompositeNode::~CompositeNode()
{
    for (Node *child : children)
    {
        delete child;
    }
    children.clear();
}

void BehaviorTree::DecoratorNode::abort()
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

void BehaviorTree::DecoratorNode::finish(BehaviorTree::Node::Result result)
{
    if (child != nullptr && child->isEntered())
    {
        throw gu_err(getNodeErrorInfo(this) + " wants to finish, but child " + getNodeErrorInfo(child) + " is entered!");
    }
    Node::finish(result);
}

BehaviorTree::DecoratorNode *BehaviorTree::DecoratorNode::setChild(BehaviorTree::Node *inChild)
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

BehaviorTree::Node *BehaviorTree::DecoratorNode::getChild() const
{
    return child;
}

BehaviorTree::DecoratorNode::~DecoratorNode()
{
    delete child;
}

constexpr static int INVALID_CHILD_INDEX = -1;

BehaviorTree::SequenceNode::SequenceNode() :
    currentChildIndex(INVALID_CHILD_INDEX)
{
}

void BehaviorTree::SequenceNode::enter()
{
    if (currentChildIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Already entered this SequenceNode: " + getNodeErrorInfo(this));
    }
    BehaviorTree::Node::enter();

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

void BehaviorTree::SequenceNode::abort()
{
    BehaviorTree::Node::abort();
#ifndef NDEBUG
    if (currentChildIndex == INVALID_CHILD_INDEX)
    {
        throw gu_err("Trying to abort a sequence that has not entered a child: " + getNodeErrorInfo(this));
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void BehaviorTree::SequenceNode::finish(BehaviorTree::Node::Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    BehaviorTree::CompositeNode::finish(result);
}

const char *BehaviorTree::SequenceNode::getName() const
{
    return "Sequence";
}

void checkCorrectChildFinished(BehaviorTree::Node *parent, const std::vector<BehaviorTree::Node *> &children,
    int currentChildIndex, BehaviorTree::Node *finishedChild)
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

void BehaviorTree::SequenceNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    BehaviorTree::CompositeNode::onChildFinished(child, result);

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

BehaviorTree::SelectorNode::SelectorNode() :
    currentChildIndex(INVALID_CHILD_INDEX)
{
}

void BehaviorTree::SelectorNode::enter()
{
    if (currentChildIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Already entered this SelectorNode: " + getNodeErrorInfo(this));
    }
    BehaviorTree::Node::enter();

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

void BehaviorTree::SelectorNode::abort()
{
    BehaviorTree::Node::abort();
#ifndef NDEBUG
    if (currentChildIndex == INVALID_CHILD_INDEX)
    {
        throw gu_err("Trying to abort a selector that has not entered a child: " + getNodeErrorInfo(this));
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void BehaviorTree::SelectorNode::finish(BehaviorTree::Node::Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    BehaviorTree::CompositeNode::finish(result);
}

const char *BehaviorTree::SelectorNode::getName() const
{
    return "Selector";
}

void BehaviorTree::SelectorNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
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

void BehaviorTree::InverterNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("InverterNode has no child: " + getNodeErrorInfo(this));
    }
    BehaviorTree::Node::enter();
    child->enter();
}

const char *BehaviorTree::InverterNode::getName() const
{
    return "Inverter";
}

void BehaviorTree::InverterNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    if (result == Node::Result::ABORTED)
    {
        finish(Node::Result::ABORTED);
        return;
    }
    finish(result == Node::Result::SUCCESS ? Node::Result::FAILURE : Node::Result::SUCCESS);
}

void BehaviorTree::SucceederNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("SucceederNode has no child: " + getNodeErrorInfo(this));
    }
    BehaviorTree::Node::enter();
    child->enter();
}

const char *BehaviorTree::SucceederNode::getName() const
{
    return "Succeeder";
}

void BehaviorTree::SucceederNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    if (result == Node::Result::ABORTED)
    {
        finish(Node::Result::ABORTED);
        return;
    }
    finish(Node::Result::SUCCESS);
}

void BehaviorTree::RepeaterNode::enter()
{
#ifndef NDEBUG
    timesRepeated = 0;
#endif
    BehaviorTree::Node::enter();
    enterChild();
}

void BehaviorTree::RepeaterNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
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
            finish(Node::Result::SUCCESS);
            break;
        case Result::ABORTED:
            finish(Node::Result::ABORTED);
            break;
    }
}

void BehaviorTree::RepeaterNode::enterChild()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("RepeaterNode has no child: " + getNodeErrorInfo(this));
    }
    child->enter();
}

const char *BehaviorTree::RepeaterNode::getName() const
{
    return "Repeater";
}

void BehaviorTree::RepeaterNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
#ifndef NDEBUG
    ImGui::Text("%dx", timesRepeated);
#endif
}

void BehaviorTree::ComponentDecoratorNode::addWhileEntered(EntityEngine *engine, entt::entity entity,
    const ComponentUtils *componentUtils)
{
    if (engine == nullptr)
    {
        throw gu_err("Engine is a nullptr! " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Entity is not valid! " + getNodeErrorInfo(this));
    }
    if (componentUtils == nullptr)
    {
        throw gu_err("ComponentUtils is a nullptr! " + getNodeErrorInfo(this));
    }
    toAddWhileEntered.push_back({engine, entity, componentUtils});
}

void BehaviorTree::ComponentDecoratorNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("ComponentDecoratorNode has no child: " + getNodeErrorInfo(this));
    }
    Node::enter();

    for (const EntityComponent entityComponent : toAddWhileEntered)
    {
        if (!entityComponent.engine->entities.valid(entityComponent.entity))
        {
            throw gu_err("Cannot add " + std::string(entityComponent.componentUtils->structInfo->name) +
                " to an invalid entity while entering: " + getNodeErrorInfo(this));
        }
        entityComponent.componentUtils->addComponent(entityComponent.entity, entityComponent.engine->entities);
    }

    child->enter();
}

void BehaviorTree::ComponentDecoratorNode::finish(BehaviorTree::Node::Result result)
{
    for (const EntityComponent entityComponent : toAddWhileEntered)
    {
        if (!entityComponent.engine->entities.valid(entityComponent.entity))
        {
            continue; // Ignore.
        }
        entityComponent.componentUtils->removeComponent(entityComponent.entity, entityComponent.engine->entities);
    }
    DecoratorNode::finish(result);
}

const char *BehaviorTree::ComponentDecoratorNode::getName() const
{
    return "ComponentDecorator";
}

void BehaviorTree::ComponentDecoratorNode::drawDebugInfo() const
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
        ImGui::Text("%s", entityComponent.componentUtils->structInfo->name);
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

void BehaviorTree::ComponentDecoratorNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    finish(result);
}

BehaviorTree::WaitNode::WaitNode() :
    seconds(-1.0f),
    waitingEntity(entt::null),
    engine(nullptr)
#ifndef NDEBUG
    ,
    timeStarted(0.0f)
#endif
{}

BehaviorTree::WaitNode *BehaviorTree::WaitNode::finishAfter(const float inSeconds, const entt::entity inWaitingEntity,
    EntityEngine *inEngine)
{
    seconds = inSeconds;
    waitingEntity = inWaitingEntity;
    engine = inEngine;
    return this;
}

void BehaviorTree::WaitNode::enter()
{
    Node::enter();
    if (seconds >= 0.0f && engine != nullptr)
    {
        onWaitFinished = engine->getTimeOuts()->callAfter(seconds, waitingEntity, [&] ()
        {
            finish(Node::Result::SUCCESS);
        });
#ifndef NDEBUG
        if (Room *room = dynamic_cast<Room *>(engine))
        {
            timeStarted = float(room->getLevel().getTime());
        }
#endif
    }
}

void BehaviorTree::WaitNode::abort()
{
    Node::abort();
    finish(Node::Result::ABORTED);
}

void BehaviorTree::WaitNode::finish(BehaviorTree::Node::Result result)
{
    onWaitFinished.reset();
    Node::finish(result);
}

const char *BehaviorTree::WaitNode::getName() const
{
    return "Wait";
}

void BehaviorTree::WaitNode::drawDebugInfo() const
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
        if (Room *room = dynamic_cast<Room *>(engine))
        {
            ImGui::SameLine();
            const float timeElapsed = float(room->getLevel().getTime()) - timeStarted;
            ImGui::ProgressBar(timeElapsed / seconds, ImVec2(100.0f, 0.0f), std::string(std::to_string(int(timeElapsed))
                + "." + std::to_string(int(fract(timeElapsed) * 10.0f)) + "s").c_str());
        }
    }
#endif

}

BehaviorTree::ComponentObserverNode::ComponentObserverNode() :
    fulfilledNodeIndex(INVALID_CHILD_INDEX),
    unfulfilledNodeIndex(INVALID_CHILD_INDEX),
    bFulFilled(false),
    currentNodeIndex(INVALID_CHILD_INDEX)
{
}

void BehaviorTree::ComponentObserverNode::enter()
{
    BehaviorTree::CompositeNode::enter();
    enterChild();
}

void BehaviorTree::ComponentObserverNode::abort()
{
    BehaviorTree::CompositeNode::abort();
    if (currentNodeIndex != INVALID_CHILD_INDEX)
    {
        getChildren().at(currentNodeIndex)->abort();
    }
    else
    {
        finish(Node::Result::ABORTED);
    }
}

void BehaviorTree::ComponentObserverNode::finish(BehaviorTree::Node::Result result)
{
    currentNodeIndex = INVALID_CHILD_INDEX;
    Node::finish(result);
}

void BehaviorTree::ComponentObserverNode::has(EntityEngine *engine, entt::entity entity,
    const ComponentUtils *componentUtils)
{
    observe(engine, entity, componentUtils, true, false);
}

void BehaviorTree::ComponentObserverNode::exclude(EntityEngine *engine, entt::entity entity,
    const ComponentUtils *componentUtils)
{
    observe(engine, entity, componentUtils, false, true);
}

BehaviorTree::ComponentObserverNode *BehaviorTree::ComponentObserverNode::setOnFulfilledNode(Node *child)
{
    if (fulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set fulfilled child for a second time: " + getNodeErrorInfo(this));
    }
    fulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

BehaviorTree::ComponentObserverNode *BehaviorTree::ComponentObserverNode::setOnUnfulfilledNode(Node *child)
{
    if (unfulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set unfulfilled child for a second time: " + getNodeErrorInfo(this));
    }
    unfulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

BehaviorTree::CompositeNode *BehaviorTree::ComponentObserverNode::addChild(BehaviorTree::Node *child)
{
    throw gu_err("Do not call addChild on this node: " + getNodeErrorInfo(this));
}

const char *BehaviorTree::ComponentObserverNode::getName() const
{
    return "ComponentObserver";
}

void BehaviorTree::ComponentObserverNode::drawDebugInfo() const
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

        const bool bHasComponent = observerHandle.componentUtils->entityHasComponent(entity,
            observerHandle.engine->entities);

        if (const char *entityName = observerHandle.engine->getName(entity))
        {
            ImGui::Text("#%d %s ", int(entity), entityName);
            ImGui::SameLine();
        }
        ImGui::TextDisabled(bCurrentValue == bHasComponent ? "has " : "excludes ");
        ImGui::SameLine();
        ImGui::Text("%s", observerHandle.componentUtils->structInfo->name);
        ImGui::SameLine();
    }
}

void BehaviorTree::ComponentObserverNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
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

void BehaviorTree::ComponentObserverNode::observe(EntityEngine *engine, entt::entity entity,
    const ComponentUtils *componentUtils, const bool presentValue, const bool absentValue)
{
    if (isEntered())
    {
        throw gu_err("Cannot edit while entered: " + getNodeErrorInfo(this));
    }
#ifndef NDEBUG
    if (engine == nullptr)
    {
        throw gu_err("No EntityEngine was given: " + getNodeErrorInfo(this));
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Invalid entity was given: " + getNodeErrorInfo(this));
    }
#endif
    // TODO: Only register callbacks on enter. Remove callbacks when leaving or in destructor
    EntityObserver *observer = componentUtils->getEntityObserver(engine->entities);
    const int conditionIndex = int(conditions.size());
    EntityObserver::Handle onConstructHandle = observer->onConstruct(entity,
        [this, engine, entity, conditionIndex, presentValue] ()
    {
        conditions[conditionIndex] = presentValue;
        onConditionsChanged(engine, entity);
    });
    EntityObserver::Handle onDestroyHandle = observer->onDestroy(entity,
        [this, engine, entity, conditionIndex, absentValue] ()
    {
        conditions[conditionIndex] = absentValue;
        onConditionsChanged(engine, entity);
    });
    observerHandles.push_back(ObserverHandle {
        engine, componentUtils, onConstructHandle
    });
    observerHandles.push_back(ObserverHandle {
        engine, componentUtils, onDestroyHandle
    });
    conditions.push_back(componentUtils->entityHasComponent(entity, engine->entities) ? presentValue : absentValue);
    bFulFilled = allConditionsFulfilled();
}

bool BehaviorTree::ComponentObserverNode::allConditionsFulfilled() const
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
void BehaviorTree::ComponentObserverNode::onConditionsChanged(EntityEngine *engine, entt::entity entity)
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

int BehaviorTree::ComponentObserverNode::getChildIndexToEnter() const
{
    return bFulFilled ? fulfilledNodeIndex : unfulfilledNodeIndex;
}

void BehaviorTree::ComponentObserverNode::enterChild()
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

BehaviorTree::ComponentObserverNode::~ComponentObserverNode()
{
    for (ObserverHandle &observerHandle : observerHandles)
    {
        if (observerHandle.engine->isDestructing())
        {
            // getEntityObserver() will crash because the registry's context variables are destroyed already.
            // Unregistering is not needed because the callbacks will be destroyed anyway.
            continue;
        }
        observerHandle.componentUtils
            ->getEntityObserver(observerHandle.engine->entities)
            ->unregister(observerHandle.handle);
    }
}

BehaviorTree::LuaLeafNode::LuaLeafNode() :
    bInEnterFunction(false)
{}

void BehaviorTree::LuaLeafNode::enter()
{
    BehaviorTree::Node::enter();
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

void BehaviorTree::LuaLeafNode::abort()
{
    BehaviorTree::Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void BehaviorTree::LuaLeafNode::finish(BehaviorTree::Node::Result result)
{
    if (bInEnterFunction && isAborted())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *BehaviorTree::LuaLeafNode::getName() const
{
    return "LuaLeaf";
}

void BehaviorTree::LuaLeafNode::finishAborted()
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

BehaviorTree::FunctionalLeafNode::FunctionalLeafNode() :
    bInEnterFunction(false)
{}

BehaviorTree::FunctionalLeafNode *
BehaviorTree::FunctionalLeafNode::setOnEnter(std::function<void(FunctionalLeafNode &)> function)
{
    onEnter = std::move(function);
    return this;
}

BehaviorTree::FunctionalLeafNode *
BehaviorTree::FunctionalLeafNode::setOnAbort(std::function<void(FunctionalLeafNode &)> function)
{
    onAbort = std::move(function);
    return this;
}

void BehaviorTree::FunctionalLeafNode::enter()
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
        finish(BehaviorTree::Node::Result::SUCCESS);
    }
}

void BehaviorTree::FunctionalLeafNode::abort()
{
    Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void BehaviorTree::FunctionalLeafNode::finish(BehaviorTree::Node::Result result)
{
    if (bInEnterFunction && isAborted())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *BehaviorTree::FunctionalLeafNode::getName() const
{
    return "FunctionalLeaf";
}

void BehaviorTree::FunctionalLeafNode::finishAborted()
{
    if (onAbort)
    {
        onAbort(*this);
    }
    else
    {
        finish(BehaviorTree::Node::Result::ABORTED);
    }
}

BehaviorTree::BehaviorTree() :
    rootNode(nullptr)
{

}

void BehaviorTree::setRootNode(BehaviorTree::Node *root)
{
    if (rootNode)
    {
        throw gu_err("A rootNode was already set!");
    }
    rootNode = std::shared_ptr<Node>(root);
}

BehaviorTree::Node *BehaviorTree::getRootNode() const
{
    return rootNode.get();
}

void BehaviorTree::addToLuaEnvironment(sol::state *lua)
{
    lua->new_enum(
        "BTResult",
        "SUCCESS", BehaviorTree::Node::Result::SUCCESS,
        "FAILURE", BehaviorTree::Node::Result::FAILURE,
        "ABORTED", BehaviorTree::Node::Result::ABORTED
    );

    // ------------------------ Abstract Node classes: -------------------------- //

    sol::usertype<BehaviorTree::Node> nodeType = lua->new_usertype<BehaviorTree::Node>(
        "BTNode",

        "finish", &BehaviorTree::Node::finish
    );

    sol::usertype<BehaviorTree::CompositeNode> compositeNodeType = lua->new_usertype<BehaviorTree::CompositeNode>(
        "BTCompositeNode",
        sol::base_classes,
        sol::bases<BehaviorTree::Node>(),

        "addChild", &BehaviorTree::CompositeNode::addChild
    );

    sol::usertype<BehaviorTree::DecoratorNode> decoratorNodeType = lua->new_usertype<BehaviorTree::DecoratorNode>(
        "BTDecoratorNode",
        sol::base_classes,
        sol::bases<BehaviorTree::Node>(),

        "setChild", &BehaviorTree::DecoratorNode::setChild
    );

    // ------------------------ Basic Node classes: -------------------------- //

    sol::usertype<BehaviorTree::SequenceNode> sequenceNodeType = lua->new_usertype<BehaviorTree::SequenceNode>(
        "BTSequenceNode",
        sol::factories([] ()
        {
            return new BehaviorTree::SequenceNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::CompositeNode>()
    );

    sol::usertype<BehaviorTree::SelectorNode> selectorNodeType = lua->new_usertype<BehaviorTree::SelectorNode>(
        "BTSelectorNode",
        sol::factories([] ()
        {
            return new BehaviorTree::SelectorNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::CompositeNode>()
    );

    sol::usertype<BehaviorTree::InverterNode> inverterNodeType = lua->new_usertype<BehaviorTree::InverterNode>(
        "BTInverterNode",
        sol::factories([] ()
        {
            return new BehaviorTree::InverterNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::DecoratorNode>()
    );

    sol::usertype<BehaviorTree::SucceederNode> succeederNodeType = lua->new_usertype<BehaviorTree::SucceederNode>(
        "BTSucceederNode",
        sol::factories([] ()
        {
            return new BehaviorTree::SucceederNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::DecoratorNode>()
    );

    sol::usertype<BehaviorTree::RepeaterNode> repeaterNodeType = lua->new_usertype<BehaviorTree::RepeaterNode>(
        "BTRepeaterNode",
        sol::factories([] ()
        {
            return new BehaviorTree::RepeaterNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::DecoratorNode>()
    );

    sol::usertype<BehaviorTree::ComponentDecoratorNode> componentDecoratorNodeType = lua->new_usertype<BehaviorTree::ComponentDecoratorNode>(
        "BTComponentDecoratorNode",
        sol::factories([] ()
        {
            return new BehaviorTree::ComponentDecoratorNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::DecoratorNode>(),

        "addWhileEntered", [] (BehaviorTree::ComponentDecoratorNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
        -> BehaviorTree::ComponentDecoratorNode &
        {
            const ComponentUtils *componentUtils = ComponentUtils::getFromLuaComponentTable(componentTable);
            node.addWhileEntered(currentEnv.env.value().get<EntityEngine *>(EntityEngine::LUA_ENV_PTR_NAME), entity, componentUtils);
            return node;
        }
    );

    sol::usertype<BehaviorTree::WaitNode> waitNodeType = lua->new_usertype<BehaviorTree::WaitNode>(
        "BTWaitNode",
        sol::factories([] ()
        {
            return new BehaviorTree::WaitNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::LeafNode>(),

        "finishAfter", [] (BehaviorTree::WaitNode &node, float seconds, entt::entity waitingEntity,
            const sol::this_environment &currentEnv)
            -> BehaviorTree::WaitNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            node.finishAfter(seconds, waitingEntity, currentEnv.env.value().get<EntityEngine *>(EntityEngine::LUA_ENV_PTR_NAME));
            return node;
        }
    );

    // ------------------------ Event-based Node classes: -------------------------- //

    sol::usertype<BehaviorTree::ComponentObserverNode> componentObserverNodeType = lua->new_usertype<BehaviorTree::ComponentObserverNode>(
        "BTComponentObserverNode",
        sol::factories([] ()
        {
            return new BehaviorTree::ComponentObserverNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::CompositeNode>(),

        "has", [] (BehaviorTree::ComponentObserverNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
            -> BehaviorTree::ComponentObserverNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            const ComponentUtils *componentUtils = ComponentUtils::getFromLuaComponentTable(componentTable);
            node.has(currentEnv.env.value().get<EntityEngine *>(EntityEngine::LUA_ENV_PTR_NAME), entity, componentUtils);
            return node;
        },
        "exclude", [] (BehaviorTree::ComponentObserverNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv)
            -> BehaviorTree::ComponentObserverNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            const ComponentUtils *componentUtils = ComponentUtils::getFromLuaComponentTable(componentTable);
            node.exclude(currentEnv.env.value().get<EntityEngine *>(EntityEngine::LUA_ENV_PTR_NAME), entity, componentUtils);
            return node;
        },
        "fulfilled", &BehaviorTree::ComponentObserverNode::setOnFulfilledNode,
        "unfulfilled", &BehaviorTree::ComponentObserverNode::setOnUnfulfilledNode
    );

    // ------------------------ Customization Node classes: -------------------------- //

    sol::usertype<BehaviorTree::LuaLeafNode> luaLeafNodeType = lua->new_usertype<BehaviorTree::LuaLeafNode>(
        "BTLuaLeafNode",
        sol::factories([] ()
        {
            return new BehaviorTree::LuaLeafNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::LeafNode>(),

        "setEnterFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> BehaviorTree::LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaEnterFunction = function;
            return luaLeafNode;
        },
        "setAbortFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> BehaviorTree::LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaAbortFunction = function;
            return luaLeafNode;
        }
    );
}
