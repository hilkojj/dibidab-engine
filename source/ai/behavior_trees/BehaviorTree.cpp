
#include "BehaviorTree.h"

void BehaviorTree::Node::registerAsParent(BehaviorTree::Node *child)
{
    if (child->parent != nullptr)
    {
        throw gu_err("Child already has a parent!");
    }
    child->parent = this;
}

void BehaviorTree::Node::finish(BehaviorTree::Node::Result result)
{
    if (parent != nullptr)
    {
        parent->onChildFinished(this, result);
    }
}

BehaviorTree::CompositeNode &BehaviorTree::CompositeNode::addChild(BehaviorTree::Node *child)
{
    if (child == nullptr)
    {
        throw gu_err("Child is null!");
    }
    registerAsParent(child);
    children.push_back(child);
    return *this;
}

const std::vector<BehaviorTree::Node *> &BehaviorTree::CompositeNode::getChildren() const
{
    return children;
}

BehaviorTree::DecoratorNode &BehaviorTree::DecoratorNode::setChild(BehaviorTree::Node *inChild)
{
    registerAsParent(inChild);
    child = inChild;
    return *this;
}

BehaviorTree::Node *BehaviorTree::DecoratorNode::getChild() const
{
    return child;
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
    // TODO
    currentChildIndex = INVALID_CHILD_INDEX;
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

    if (result != Node::Result::SUCCESS)
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
    // TODO
    currentChildIndex = INVALID_CHILD_INDEX;
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

    if (result == Node::Result::SUCCESS)
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
    child->enter();
}

void BehaviorTree::InverterNode::abort()
{
    // TODO
}

void BehaviorTree::InverterNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    // TODO: what to return in case of termination?
    finish(result == Node::Result::SUCCESS ? Node::Result::FAILURE : Node::Result::SUCCESS);
}

void BehaviorTree::SucceederNode::enter()
{
    Node *child = getChild();
    if (child == nullptr)
    {
        throw gu_err("SucceederNode has no child!");
    }
    child->enter();
}

void BehaviorTree::SucceederNode::abort()
{
    // TODO
}

void BehaviorTree::SucceederNode::onChildFinished(BehaviorTree::Node *child, BehaviorTree::Node::Result result)
{
    Node::onChildFinished(child, result);
    finish(Node::Result::SUCCESS);
}

BehaviorTree::LuaLeafNode::LuaLeafNode() :
        bEntered(false)
{
}

void BehaviorTree::LuaLeafNode::enter()
{
    bEntered = true;
    if (luaEnterFunction.valid())
    {
        sol::protected_function_result result = luaEnterFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::enter failed:\n" << result.get<sol::error>().what() << std::endl;

            if (bEntered)
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
    bEntered = false;
}

void BehaviorTree::LuaLeafNode::finish(BehaviorTree::Node::Result result)
{
    bEntered = false;
    Node::finish(result);
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
    rootNode = root;
}

BehaviorTree::Node *BehaviorTree::getRootNode() const
{
    return rootNode;
}

void BehaviorTree::addToLuaEnvironment(sol::state *lua)
{
    lua->new_enum(
        "BTResult",
        "SUCCESS", BehaviorTree::Node::Result::SUCCESS,
        "FAILURE", BehaviorTree::Node::Result::FAILURE
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
        {
            luaLeafNode.luaEnterFunction = function;
            return luaLeafNode;
        }
    );
}

