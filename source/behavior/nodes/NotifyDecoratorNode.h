#pragma once

#include "../Tree.h"

#include <functional>

namespace dibidab::behavior
{
    struct NotifyDecoratorNode : public Tree::DecoratorNode
    {
        NotifyDecoratorNode();

        NotifyDecoratorNode *setOnEnter(std::function<void()> function);

        NotifyDecoratorNode *setOnFinish(std::function<void(Result)> function);

        void enter() override;

        void finish(Result result) override;

        void onChildFinished(Node *child, Result result) override;

        const char *getName() const override;

      private:
        std::function<void()> onEnter;
        std::function<void(Result)> onFinish;
    };
}
