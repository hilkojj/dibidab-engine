#include "NotifyDecoratorNode.h"

#include <utils/gu_error.h>

dibidab::behavior::NotifyDecoratorNode::NotifyDecoratorNode()
{

}

dibidab::behavior::NotifyDecoratorNode *dibidab::behavior::NotifyDecoratorNode::setOnEnter(std::function<void()> function)
{
    this->onEnter = function;
    return this;
}

dibidab::behavior::NotifyDecoratorNode * dibidab::behavior::NotifyDecoratorNode::setOnFinish(std::function<void(Result)> function)
{
    this->onFinish = function;
    return this;
}

void dibidab::behavior::NotifyDecoratorNode::enter()
{
    if (this->onEnter)
    {
        this->onEnter();
    }
    Node::enter();
    if (getChild() == nullptr)
    {
        throw gu_err("NotifyDecoratorNode has no child!");
    }
    getChild()->enter();
}

void dibidab::behavior::NotifyDecoratorNode::finish(dibidab::behavior::Tree::Node::Result result)
{
    DecoratorNode::finish(result);
    if (this->onFinish)
    {
        this->onFinish(result);
    }
}

void dibidab::behavior::NotifyDecoratorNode::onChildFinished(Node *child, Result result)
{
    Node::onChildFinished(child, result);
    finish(result);
}

const char *dibidab::behavior::NotifyDecoratorNode::getName() const
{
    return "NotifyDecoratorNode";
}
