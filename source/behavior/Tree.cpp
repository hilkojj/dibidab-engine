#include "Tree.h"

#include "nodes/ComponentDecoratorNode.h"
#include "nodes/ComponentObserverNode.h"
#include "nodes/LuaLeafNode.h"
#include "nodes/WaitNode.h"

#include "../level/room/Room.h"
#include "../reflection/ComponentInfo.h"

#include <utils/string_utils.h>

#include <sol/sol.hpp>
#include <imgui.h>

dibidab::behavior::Tree::Node::Node() :
        parent(nullptr),
        bEntered(false),
        bAborting(false)
{
    lua_State *luaState = luau::getLuaState().lua_state();
    lua_Debug luaDebugInfo;
    if (lua_getstack(luaState, 1, &luaDebugInfo))
    {
        if (lua_getinfo(luaState, "nSl", &luaDebugInfo))
        {
            source.path = luaDebugInfo.source;
            source.line = luaDebugInfo.currentline;
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
        throw gu_err("Cannot abort a non-entered Node: " + getReadableDebugInfo());
    }
    if (bAborting)
    {
        throw gu_err("Cannot abort a Node again: " + getReadableDebugInfo());
    }
#endif
    bAborting = true;
}

void dibidab::behavior::Tree::Node::finish(Result result)
{
#ifndef NDEBUG
    if (!bEntered)
    {
        throw gu_err("Cannot finish a non-entered Node: " + getReadableDebugInfo());
    }
#endif
    if (bAborting)
    {
        if (result != Result::ABORTED)
        {
            throw gu_err("Cannot finish an aborted Node with an result other than ABORTED: " + getReadableDebugInfo());
        }
        bAborting = false;
    }
    bEntered = false;
    bFinishedAtLeastOnce = true;
    lastResult = result;
    if (parent != nullptr)
    {
        parent->onChildFinished(this, result);
    }
}

bool dibidab::behavior::Tree::Node::isEntered() const
{
    return bEntered;
}

bool dibidab::behavior::Tree::Node::isAborting() const
{
    return bAborting;
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

std::string dibidab::behavior::Tree::Node::getReadableDebugInfo() const
{
    std::string info = getName();
    if (source.path != nullptr)
    {
        std::vector<std::string> pathSplitted = su::split(source.path, "/");
        if (!pathSplitted.empty())
        {
            info += "@" + pathSplitted.back() + ":" + std::to_string(source.line);
        }
    }
    return info;
}

bool dibidab::behavior::Tree::Node::hasFinishedAtLeastOnce() const
{
    return bFinishedAtLeastOnce;
}

dibidab::behavior::Tree::Node::Result dibidab::behavior::Tree::Node::getLastResult() const
{
    return lastResult;
}

void dibidab::behavior::Tree::Node::registerAsParent(Node *child)
{
    if (child->parent != nullptr)
    {
        throw gu_err("Child already has a parent: " + child->getReadableDebugInfo());
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
            throw gu_err(getReadableDebugInfo() + " wants to finish, but child " + child->getReadableDebugInfo() + " is entered!");
        }
    }
#endif
    Node::finish(result);
}

dibidab::behavior::Tree::CompositeNode *dibidab::behavior::Tree::CompositeNode::addChild(Node *child)
{
    if (child == nullptr)
    {
        throw gu_err("Child is null: " + getReadableDebugInfo());
    }
    registerAsParent(child);
    children.push_back(child);
    return this;
}

const std::vector<dibidab::behavior::Tree::Node *> &dibidab::behavior::Tree::CompositeNode::getChildren() const
{
    return children;
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

dibidab::behavior::Tree::CompositeNode::~CompositeNode()
{
    for (Node *child : children)
    {
        delete child;
    }
    children.clear();
}

void dibidab::behavior::Tree::CompositeNode::checkCorrectChildFinished(
    const int expectedChildIndex, const dibidab::behavior::Tree::Node *finishedChild
) const
{
    if (expectedChildIndex == INVALID_CHILD_INDEX || !isEntered())
    {
        if (!isEntered())
        {
            throw gu_err("Child (" + finishedChild->getReadableDebugInfo() + ") finished, but parent (" + getReadableDebugInfo() + ") was not entered!");
        }
        else
        {
            throw gu_err("Child (" + finishedChild->getReadableDebugInfo() + ") finished, but parent (" + getReadableDebugInfo() + ") thinks no child was entered!");
        }
    }
    if (finishedChild != children[expectedChildIndex])
    {
        throw gu_err("Child that finished is not the current child that was entered!\nFinished child: " + finishedChild->getReadableDebugInfo()
            + "\nEntered child: " + children[expectedChildIndex]->getReadableDebugInfo() + "\nParent: " + getReadableDebugInfo());
    }
}

void dibidab::behavior::Tree::DecoratorNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("DecoratorNode has no child: " + getReadableDebugInfo());
    }
    Node::enter();
    child->enter();
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
        throw gu_err(getReadableDebugInfo() + " wants to finish, but child " + child->getReadableDebugInfo() + " is entered!");
    }
    Node::finish(result);
}

dibidab::behavior::Tree::DecoratorNode *dibidab::behavior::Tree::DecoratorNode::setChild(Node *inChild)
{
    if (child != nullptr)
    {
        if (isEntered())
        {
            throw gu_err("Cannot set child on a Decorator Node that is currently entered: " + getReadableDebugInfo());
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

const char *dibidab::behavior::Tree::DecoratorNode::getName() const
{
    return "Decorator";
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
        throw gu_err("Already entered this SequenceNode: " + getReadableDebugInfo());
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
        throw gu_err("Trying to abort a sequence that has not entered a child: " + getReadableDebugInfo());
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

void dibidab::behavior::Tree::SequenceNode::onChildFinished(Node *child, Result result)
{
    Tree::CompositeNode::onChildFinished(child, result);

    const std::vector<Node *> &children = getChildren();

    checkCorrectChildFinished(currentChildIndex, child);

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
        throw gu_err("Already entered this SelectorNode: " + getReadableDebugInfo());
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
        throw gu_err("Trying to abort a selector that has not entered a child: " + getReadableDebugInfo());
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

    checkCorrectChildFinished(currentChildIndex, child);

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
        CompositeNode::finish(isAborting() ? Node::Result::ABORTED : Node::Result::SUCCESS);
    }
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
    // Do not cal DecoratorNode::enter, because we want to succeed if there's no child!
    Node::enter();
    if (Node *child = getChild())
    {
        child->enter();
    }
    else
    {
        finish(Result::SUCCESS);
    }
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

void dibidab::behavior::Tree::FailNode::enter()
{
    // Do not cal DecoratorNode::enter, because we want to succeed if there's no child!
    Node::enter();
    if (Node *child = getChild())
    {
        child->enter();
    }
    else
    {
        finish(Result::FAILURE);
    }
}

const char *dibidab::behavior::Tree::FailNode::getName() const
{
    return "Fail";
}

void dibidab::behavior::Tree::FailNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    if (result == Result::ABORTED)
    {
        finish(Result::ABORTED);
        return;
    }
    finish(Result::FAILURE);
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
        throw gu_err("RepeaterNode has no child: " + getReadableDebugInfo());
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

dibidab::behavior::Tree::Tree() :
    rootNode(nullptr)
{

}

void dibidab::behavior::Tree::setRootNode(Node *root)
{
    if (rootNode && rootNode->isEntered())
    {
        throw gu_err("Cannot change root while it's entered!");
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

    const auto nodeType = lua->new_usertype<Node>(
        "BTNode",

        "finish", &Node::finish,
        "setDescription", &Node::setDescription
    );

    const auto compositeNodeType = lua->new_usertype<CompositeNode>(
        "BTComposite",
        sol::base_classes,
        sol::bases<Node>(),

        "addChild", &CompositeNode::addChild
    );

    const auto decoratorNodeType = lua->new_usertype<DecoratorNode>(
        "BTDecorator",
        sol::factories([] ()
        {
            return new DecoratorNode();
        }),
        sol::base_classes,
        sol::bases<Node>(),

        "setChild", &DecoratorNode::setChild
    );

    // ------------------------ Basic Node classes: -------------------------- //

    const auto sequenceNodeType = lua->new_usertype<SequenceNode>(
        "BTSequence",
        sol::factories([] ()
        {
            return new SequenceNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    const auto selectorNodeType = lua->new_usertype<SelectorNode>(
        "BTSelector",
        sol::factories([] ()
        {
            return new SelectorNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    const auto parallelNodeType = lua->new_usertype<ParallelNode>(
        "BTParallel",
        sol::factories([] ()
        {
            return new ParallelNode();
        }),
        sol::base_classes,
        sol::bases<Node, CompositeNode>()
    );

    const auto inverterNodeType = lua->new_usertype<InverterNode>(
        "BTInverter",
        sol::factories([] ()
        {
            return new InverterNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    const auto succeederNodeType = lua->new_usertype<SucceederNode>(
        "BTSucceeder",
        sol::factories([] ()
        {
            return new SucceederNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );
    const auto failNodeType = lua->new_usertype<FailNode>(
        "BTFail",
        sol::factories([] ()
        {
            return new FailNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    const auto repeaterNodeType = lua->new_usertype<RepeaterNode>(
        "BTRepeaterNode",
        sol::factories([] ()
        {
            return new RepeaterNode();
        }),
        sol::base_classes,
        sol::bases<Node, DecoratorNode>()
    );

    const auto componentDecoratorNodeType = lua->new_usertype<ComponentDecoratorNode>(
        "BTComponentDecorator",
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

    const auto waitNodeType = lua->new_usertype<WaitNode>(
        "BTWait",
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

    const auto componentObserverNodeType = lua->new_usertype<ComponentObserverNode>(
        "BTComponentObserver",
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

    const auto luaLeafNodeType = lua->new_usertype<LuaLeafNode>(
        "BTLuaLeaf",
        sol::factories([] ()
        {
            return new LuaLeafNode();
        }),
        sol::base_classes,
        sol::bases<Node, LeafNode>(),

        "onEnter", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaEnterFunction = function;
            return luaLeafNode;
        },
        "onAbort", [] (LuaLeafNode &luaLeafNode, const sol::function &function)
            -> LuaLeafNode & // Important! Explicitly saying it returns a reference to this node to prevent segfaults.
        {
            luaLeafNode.luaAbortFunction = function;
            return luaLeafNode;
        }
    );
}
