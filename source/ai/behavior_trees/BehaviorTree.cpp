
#include "BehaviorTree.h"

BehaviorTree::Node::Node() :
    parent(nullptr),
    bEntered(false),
    bAborted(false)
{
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
        throw gu_err("Cannot abort a non-entered Node!");
    }
    if (bAborted)
    {
        throw gu_err("Cannot abort a Node again!");
    }
#endif
    bAborted = true;
}

void BehaviorTree::Node::finish(BehaviorTree::Node::Result result)
{
#ifndef NDEBUG
    if (!bEntered)
    {
        throw gu_err("Cannot finish a non-entered Node!");
    }
#endif
    if (bAborted)
    {
        if (result != Result::ABORTED)
        {
            throw gu_err("Cannot finish an aborted Node with an result other than ABORTED!");
        }
        bAborted = false;
    }
    bEntered = false;
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

void BehaviorTree::Node::registerAsParent(BehaviorTree::Node *child)
{
    if (child->parent != nullptr)
    {
        throw gu_err("Child already has a parent!");
    }
    child->parent = this;
}

BehaviorTree::CompositeNode *BehaviorTree::CompositeNode::addChild(BehaviorTree::Node *child)
{
    if (child == nullptr)
    {
        throw gu_err("Child is null!");
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
}

BehaviorTree::DecoratorNode *BehaviorTree::DecoratorNode::setChild(BehaviorTree::Node *inChild)
{
    if (child != nullptr)
    {
        if (isEntered())
        {
            throw gu_err("Cannot set child on a Decorator Node that is currently entered!");
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
        throw gu_err("Already entered this SequenceNode!");
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
        throw gu_err("Trying to abort a sequence that has not entered a child.");
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void BehaviorTree::SequenceNode::finish(BehaviorTree::Node::Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    BehaviorTree::Node::finish(result);
}

void BehaviorTree::SequenceNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);

    const std::vector<Node *> &children = getChildren();

    assert(currentChildIndex < children.size());

    if (child != children[currentChildIndex])
    {
        throw gu_err("Child that finished is not the current child that was entered!");
    }

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
        throw gu_err("Already entered this SelectorNode!");
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
        throw gu_err("Trying to abort a selector that has not entered a child.");
    }
#endif
    getChildren().at(currentChildIndex)->abort();
}

void BehaviorTree::SelectorNode::finish(BehaviorTree::Node::Result result)
{
    currentChildIndex = INVALID_CHILD_INDEX;
    Node::finish(result);
}

void BehaviorTree::SelectorNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);

    const std::vector<Node *> &children = getChildren();

    assert(currentChildIndex < children.size());

    if (child != children[currentChildIndex])
    {
        throw gu_err("Child that finished is not the current child that was entered!");
    }

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
        throw gu_err("InverterNode has no child!");
    }
    BehaviorTree::Node::enter();
    child->enter();
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
        throw gu_err("SucceederNode has no child!");
    }
    BehaviorTree::Node::enter();
    child->enter();
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

void BehaviorTree::WaitNode::abort()
{
    Node::abort();
    finish(Node::Result::ABORTED);
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
    onConditionsChanged(true);
}

void BehaviorTree::ComponentObserverNode::abort()
{
    BehaviorTree::CompositeNode::abort();
    if (currentNodeIndex != INVALID_CHILD_INDEX)
    {
        getChildren().at(currentNodeIndex)->abort();
    }
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
        throw gu_err("Cannot set fulfilled child for a second time.");
    }
    fulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

BehaviorTree::ComponentObserverNode *BehaviorTree::ComponentObserverNode::setOnUnfulfilledNode(Node *child)
{
    if (unfulfilledNodeIndex != INVALID_CHILD_INDEX)
    {
        throw gu_err("Cannot set unfulfilled child for a second time.");
    }
    unfulfilledNodeIndex = int(getChildren().size());
    CompositeNode::addChild(child);
    return this;
}

BehaviorTree::CompositeNode *BehaviorTree::ComponentObserverNode::addChild(BehaviorTree::Node *child)
{
    throw gu_err("Do not call addChild on this node.");
}

void BehaviorTree::ComponentObserverNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    if (result == Node::Result::ABORTED)
    {
        if (isAborted())
        {
            finish(Node::Result::ABORTED);
        }
        else
        {
            enterChild();
        }
    }
    else
    {
        finish(result);
    }
}

void BehaviorTree::ComponentObserverNode::observe(EntityEngine *engine, entt::entity entity,
    const ComponentUtils *componentUtils, const bool presentValue, const bool absentValue)
{
#ifndef NDEBUG
    if (engine == nullptr)
    {
        throw gu_err("No EntityEngine was given");
    }
    if (!engine->entities.valid(entity))
    {
        throw gu_err("Invalid entity was given");
    }
#endif
    // TODO: Only register callbacks on enter. Remove callbacks when leaving or in destructor
    EntityObserver *observer = componentUtils->getEntityObserver(engine->entities);
    const int conditionIndex = int(conditions.size());
    EntityObserver::Handle onConstructHandle = observer->onConstruct(entity, [this, conditionIndex, presentValue] () {
        conditions[conditionIndex] = presentValue;
        onConditionsChanged();
    });
    EntityObserver::Handle onDestroyHandle = observer->onDestroy(entity, [this, conditionIndex, absentValue] () {
        conditions[conditionIndex] = absentValue;
        onConditionsChanged();
    });
    observerHandles.push_back(ObserverHandle {
        engine, componentUtils, onConstructHandle
    });
    observerHandles.push_back(ObserverHandle {
        engine, componentUtils, onDestroyHandle
    });
    conditions.push_back(componentUtils->entityHasComponent(entity, engine->entities) ? presentValue : absentValue);
}

void BehaviorTree::ComponentObserverNode::onConditionsChanged(const bool bForceEnter)
{
    bool newFulfilled = true;
    for (const bool condition : conditions)
    {
        if (!condition)
        {
            newFulfilled = false;
            break;
        }
    }
    if (newFulfilled != bFulFilled || bForceEnter)
    {
        bFulFilled = newFulfilled;

        if (isEntered())
        {
            if (currentNodeIndex == INVALID_CHILD_INDEX)
            {
                enterChild();
            }
            else
            {
                Node *toAbort = getChildren().at(currentNodeIndex);
                if (!toAbort->isAborted())
                {
                    toAbort->abort();
                }
            }
        }
#ifndef NDEBUG
        else
        {
            assert(!bForceEnter);
        }
#endif
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


void BehaviorTree::LuaLeafNode::enter()
{
    BehaviorTree::Node::enter();
    if (luaEnterFunction.valid())
    {
        sol::protected_function_result result = luaEnterFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::enter failed:\n" << result.get<sol::error>().what() << std::endl;

            if (isEntered())
            {
                finish(Node::Result::FAILURE);
            }
        }
    }
    else
    {
        std::cerr << "LuaLeafNode::enter failed because no enter function was set!" << std::endl;
        finish(Node::Result::FAILURE);
    }
}

void BehaviorTree::LuaLeafNode::abort()
{
    BehaviorTree::Node::abort();
    if (luaAbortFunction.valid())
    {
        sol::protected_function_result result = luaAbortFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::abort failed:\n" << result.get<sol::error>().what() << std::endl;

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

    sol::usertype<BehaviorTree::WaitNode> waitNodeType = lua->new_usertype<BehaviorTree::WaitNode>(
        "BTWaitNode",
        sol::factories([] ()
        {
            return new BehaviorTree::WaitNode();
        }),
        sol::base_classes,
        sol::bases<BehaviorTree::Node, BehaviorTree::LeafNode>()
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
            const sol::this_environment &currentEnv) -> BehaviorTree::ComponentObserverNode &
        {
            const ComponentUtils *componentUtils = ComponentUtils::getFromLuaComponentTable(componentTable);
            node.has(currentEnv.env.value().get<EntityEngine *>(EntityEngine::LUA_ENV_PTR_NAME), entity, componentUtils);
            return node;
        },
        "exclude", [] (BehaviorTree::ComponentObserverNode &node, entt::entity entity, const sol::table &componentTable,
            const sol::this_environment &currentEnv) -> BehaviorTree::ComponentObserverNode &
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

        "setEnterFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function) -> BehaviorTree::LuaLeafNode &
        {
            luaLeafNode.luaEnterFunction = function;
            return luaLeafNode;
        },
        "setAbortFunction", [] (LuaLeafNode &luaLeafNode, const sol::function &function) -> BehaviorTree::LuaLeafNode &
        {
            luaLeafNode.luaAbortFunction = function;
            return luaLeafNode;
        }
    );
}
