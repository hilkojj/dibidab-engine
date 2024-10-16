#pragma once
#include "BehaviorTree.h"

class EntityEngine;

class BehaviorTreeInspector
{
  public:
    BehaviorTreeInspector(EntityEngine &engine, entt::entity entity);

    bool drawGUI();

  private:

    void drawNode(BehaviorTree::Node *node, uint depth);

    EntityEngine *engine;
    entt::entity entity;
};
