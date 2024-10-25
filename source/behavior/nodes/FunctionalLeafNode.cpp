#include "FunctionalLeafNode.h"

dibidab::behavior::FunctionalLeafNode::FunctionalLeafNode() :
    bInEnterFunction(false)
{}

dibidab::behavior::FunctionalLeafNode *dibidab::behavior::FunctionalLeafNode::setOnEnter(
    std::function<void(FunctionalLeafNode &)> function
)
{
    onEnter = std::move(function);
    return this;
}

dibidab::behavior::FunctionalLeafNode *dibidab::behavior::FunctionalLeafNode::setOnAbort(
    std::function<void(FunctionalLeafNode &)> function
)
{
    onAbort = std::move(function);
    return this;
}

void dibidab::behavior::FunctionalLeafNode::enter()
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

void dibidab::behavior::FunctionalLeafNode::abort()
{
    Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void dibidab::behavior::FunctionalLeafNode::finish(Result result)
{
    if (bInEnterFunction && isAborted())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *dibidab::behavior::FunctionalLeafNode::getName() const
{
    return "FunctionalLeaf";
}

void dibidab::behavior::FunctionalLeafNode::finishAborted()
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
