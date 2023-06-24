
#ifndef GAME_BEHAVIORTREE_H
#define GAME_BEHAVIORTREE_H

#include "../../ecs/EntityEngine.h"

#include "../../luau.h"

#include <vector>

class BehaviorTree
{
  public:

    // ------------------------ Abstract Node classes: -------------------------- //

    struct Node
    {
        enum class Result
        {
            SUCCESS,
            FAILURE,
            ABORTED
        };

        Node();

        virtual void enter();

        virtual void abort();

        virtual void finish(Result result);

        bool isEntered() const;

        bool isAborted() const;

        // TODO: delete children:
        virtual ~Node() = default;

      protected:

        void registerAsParent(Node *child);

        virtual void onChildFinished(Node *child, Result result) {};

      private:
        Node *parent;
        bool bEntered;
        bool bAborted;
    };

    struct CompositeNode : public Node
    {
        // TODO: calling this in a running tree.
        virtual CompositeNode &addChild(Node *child);

        const std::vector<Node *> &getChildren() const;

      private:
        std::vector<Node *> children;
    };

    struct DecoratorNode : public Node
    {
        void abort() override;

        // TODO: calling this in a running tree.
        virtual DecoratorNode &setChild(Node *child);

        Node *getChild() const;

      private:
        Node *child;
    };

    struct LeafNode : public Node
    {};

    // ------------------------ Basic Node classes: -------------------------- //

    struct SequenceNode : public CompositeNode
    {
        SequenceNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:
        int currentChildIndex;
    };

    struct SelectorNode : public CompositeNode
    {
        SelectorNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:
        int currentChildIndex;
    };

    struct InverterNode : public DecoratorNode
    {
        void enter() override;

      protected:
        void onChildFinished(Node *child, Result result) override;
    };

    struct SucceederNode : public DecoratorNode
    {
        void enter() override;

      protected:
        void onChildFinished(Node *child, Result result) override;
    };

    struct WaitNode : public LeafNode
    {
        void abort() override;
    };

    // ------------------------ Event-based Node classes: -------------------------- //

    struct ComponentObserverNode : public CompositeNode
    {
        ComponentObserverNode();

        void enter() override;

        void abort() override;

        template<class Component>
        void has(EntityEngine *engine, entt::entity entity)
        {
            has(engine, entity, ComponentUtils::getFor<Component>());
        }

        void has(EntityEngine *engine, entt::entity entity, const ComponentUtils *componentUtils);

        template<class Component>
        void exclude(EntityEngine *engine, entt::entity entity)
        {
            exclude(engine, entity, ComponentUtils::getFor<Component>());
        }

        void exclude(EntityEngine *engine, entt::entity entity, const ComponentUtils *componentUtils);

        ComponentObserverNode &setOnFulfilledNode(Node *child);

        ComponentObserverNode &setOnUnfulfilledNode(Node *child);

        // Do not use:
        CompositeNode &addChild(Node *child) override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:

        void onConditionsChanged(bool bForceEnter = false);

        int getChildIndexToEnter() const;

        void enterChild();

        std::vector<bool> conditions;
        int fulfilledNodeIndex;
        int unfulfilledNodeIndex;
        bool bFulFilled;
        int currentNodeIndex;
    };

    // ------------------------ Customization Node classes: -------------------------- //

    struct LuaLeafNode : public LeafNode
    {
        void enter() override;

        void abort() override;

        sol::function luaEnterFunction;
        sol::function luaAbortFunction;
    };

    BehaviorTree();

    void setRootNode(Node *root);

    Node *getRootNode() const;

  private:

    Node *rootNode;

  public:
    static void addToLuaEnvironment(sol::state *lua);
};


#endif //GAME_BEHAVIORTREE_H
