#pragma once

#include "../Tree.h"

#include <functional>

namespace dibidab::behavior
{
    struct FunctionalLeafNode : public Tree::LeafNode
    {
        FunctionalLeafNode();

        FunctionalLeafNode *setOnEnter(std::function<void(FunctionalLeafNode &node)> function);

        FunctionalLeafNode *setOnAbort(std::function<void(FunctionalLeafNode &node)> function);

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        const char *getName() const override;

      private:
        void finishAborted();

        std::function<void(FunctionalLeafNode &node)> onEnter;
        std::function<void(FunctionalLeafNode &node)> onAbort;

        bool bInEnterFunction;
    };
}
