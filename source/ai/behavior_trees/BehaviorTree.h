
#ifndef GAME_BEHAVIORTREE_H
#define GAME_BEHAVIORTREE_H

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
            FAILURE
        };

        virtual void enter() = 0;

        virtual void abort() = 0;

        virtual void finish(Result result);

        virtual ~Node() = default;

      protected:

        void registerAsParent(Node *child);

        virtual void onChildFinished(Node *child, Result result) {};

        Node *parent = nullptr;
    };

    struct CompositeNode : public Node
    {
        CompositeNode &addChild(Node *child);

        const std::vector<Node *> &getChildren() const;

      private:
        std::vector<Node *> children;
    };

    struct DecoratorNode : public Node
    {
        DecoratorNode &setChild(Node *child);

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

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:
        int currentChildIndex;
    };

    struct InverterNode : public DecoratorNode
    {
        void enter() override;

        void abort() override;

      protected:
        void onChildFinished(Node *child, Result result) override;
    };

    struct SucceederNode : public DecoratorNode
    {
        void enter() override;

        void abort() override;

      protected:
        void onChildFinished(Node *child, Result result) override;
    };

    // ------------------------ Customization Node classes: -------------------------- //

    struct LuaLeafNode : public LeafNode
    {
        LuaLeafNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        sol::function luaEnterFunction;

      private:
        bool bEntered;
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
