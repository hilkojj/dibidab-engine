#include "ComponentObserverNode.h"

#include "../../ecs/systems/TimeOutSystem.h"

#include <imgui.h>

dibidab::behavior::ComponentObserverNode::ComponentObserverNode() :
    bUseSafetyDelay(true),
    fulfilledNodeIndex(INVALID_CHILD_INDEX),
    unfulfilledNodeIndex(INVALID_CHILD_INDEX),
    bFulFilled(false),
    currentNodeIndex(INVALID_CHILD_INDEX)
{
}

void dibidab::behavior::ComponentObserverNode::enter()
{
    CompositeNode::enter();
    enterChild();
}

void dibidab::behavior::ComponentObserverNode::abort()
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

void dibidab::behavior::ComponentObserverNode::finish(Result result)
{
    currentNodeIndex = INVALID_CHILD_INDEX;
    Node::finish(result);
}

dibidab::behavior::ComponentObserverNode *dibidab::behavior::ComponentObserverNode::withoutSafetyDelay()
{
    if (!observerHandles.empty())
    {
        throw gu_err("Safety delay can only be disabled before observers are added: " + getReadableDebugInfo());
    }
    bUseSafetyDelay = false;
    return this;
}

void dibidab::behavior::ComponentObserverNode::has(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    observe(engine, entity, component, true, false);
}

void dibidab::behavior::ComponentObserverNode::exclude(ecs::Engine *engine, entt::entity entity,
    const ComponentInfo *component)
{
    observe(engine, entity, component, false, true);
}

dibidab::behavior::ComponentObserverNode *dibidab::behavior::ComponentObserverNode::setOnFulfilledNode(Node *child)
{
    if (fulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set fulfilled child for a second time: " + getReadableDebugInfo());
    }
    fulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

dibidab::behavior::ComponentObserverNode *dibidab::behavior::ComponentObserverNode::setOnUnfulfilledNode(Node *child)
{
    if (unfulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set unfulfilled child for a second time: " + getReadableDebugInfo());
    }
    unfulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

dibidab::behavior::Tree::CompositeNode *dibidab::behavior::ComponentObserverNode::addChild(Node *child)
{
    throw gu_err("Do not call addChild on this node: " + getReadableDebugInfo());
}

const char *dibidab::behavior::ComponentObserverNode::getName() const
{
    return "ComponentObserver";
}

void dibidab::behavior::ComponentObserverNode::drawDebugInfo() const
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

void dibidab::behavior::ComponentObserverNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    if (result == Node::Result::ABORTED)
    {
        if (isAborting())
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
        checkCorrectChildFinished(currentNodeIndex, child);
        finish(result);
    }
}

void dibidab::behavior::ComponentObserverNode::observe(ecs::Engine *engine, entt::entity entity,
    const dibidab::ComponentInfo *component, const bool presentValue, const bool absentValue)
{
    if (isEntered())
    {
        throw gu_err("Cannot edit while entered: " + getReadableDebugInfo());
    }
#ifndef NDEBUG
    if (component == nullptr)
    {
        throw gu_err("dibidab::ComponentInfo is a nullptr: " + getReadableDebugInfo());
    }
    if (engine == nullptr)
    {
        throw gu_err("No Engine was given: " + getReadableDebugInfo());
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Invalid entity was given: " + getReadableDebugInfo());
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

bool dibidab::behavior::ComponentObserverNode::allConditionsFulfilled() const
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
void dibidab::behavior::ComponentObserverNode::onConditionsChanged(ecs::Engine *engine, entt::entity entity)
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
                if (!toAbort->isAborting())
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

int dibidab::behavior::ComponentObserverNode::getChildIndexToEnter() const
{
    return bFulFilled ? fulfilledNodeIndex : unfulfilledNodeIndex;
}

void dibidab::behavior::ComponentObserverNode::enterChild()
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

dibidab::behavior::ComponentObserverNode::~ComponentObserverNode()
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
