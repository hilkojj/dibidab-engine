#pragma once
#include "Tree.h"

namespace dibidab
{
class EntityEngine;

class TreeInspector
{
  public:
    TreeInspector(EntityEngine &engine, entt::entity entity);

    bool drawGUI();

  private:

    void drawNode(Tree::Node *node, uint depth);

    EntityEngine *engine;
    entt::entity entity;
};

}
