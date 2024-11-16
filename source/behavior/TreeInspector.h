#pragma once
#include "Tree.h"

namespace dibidab::behavior
{
    class TreeInspector
    {
      public:
        bool drawGUI(Tree &tree, const std::string &title);

      private:

        void drawNode(Tree::Node *node, uint depth);
    };
}
