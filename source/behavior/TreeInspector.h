#pragma once
#include "Tree.h"

namespace dibidab::ecs
{
    class Engine;
}

namespace dibidab::behavior
{
    class TreeInspector
    {
      public:
        TreeInspector(ecs::Engine &engine, entt::entity entity);

        bool drawGUI();

      private:

        void drawNode(Tree::Node *node, uint depth);

        ecs::Engine *engine;
        entt::entity entity;
    };
}
