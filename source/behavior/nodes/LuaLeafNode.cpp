#include "LuaLeafNode.h"

dibidab::behavior::LuaLeafNode::LuaLeafNode() :
    bInEnterFunction(false)
{}

void dibidab::behavior::LuaLeafNode::enter()
{
    Node::enter();
    if (luaEnterFunction.valid())
    {
        bInEnterFunction = true;
        sol::protected_function_result result = luaEnterFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::enter failed for: " << getReadableDebugInfo() << "\n" << result.get<sol::error>().what() << std::endl;

            if (isEntered())
            {
                // note: will be ignored in case we're aborted and because bInEnterFunction is still true!
                finish(Node::Result::FAILURE);
            }
        }

        bInEnterFunction = false;

        if (isAborting())
        {
            finishAborted();
        }
    }
    else
    {
        std::cerr << "LuaLeafNode::enter failed because no enter function was set! " << getReadableDebugInfo() << std::endl;
        finish(Node::Result::FAILURE);
    }
}

void dibidab::behavior::LuaLeafNode::abort()
{
    Node::abort();
    if (!bInEnterFunction)
    {
        finishAborted();
    }
}

void dibidab::behavior::LuaLeafNode::finish(Result result)
{
    if (bInEnterFunction && isAborting())
    {
        // ignore this result. We'll abort after the enter function is done ;)
        return;
    }
    Node::finish(result);
}

const char *dibidab::behavior::LuaLeafNode::getName() const
{
    return "LuaLeaf";
}

void dibidab::behavior::LuaLeafNode::finishAborted()
{
    if (luaAbortFunction.valid())
    {
        sol::protected_function_result result = luaAbortFunction(this);

        if (!result.valid())
        {
            std::cerr << "LuaLeafNode::abort failed for: " << getReadableDebugInfo() << "\n" << result.get<sol::error>().what() << std::endl;

            if (isAborting())
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
