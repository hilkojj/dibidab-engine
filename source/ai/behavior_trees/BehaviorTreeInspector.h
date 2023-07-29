
#ifndef GAME_BEHAVIORTREEINSPECTOR_H
#define GAME_BEHAVIORTREEINSPECTOR_H

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

#endif //GAME_BEHAVIORTREEINSPECTOR_H
