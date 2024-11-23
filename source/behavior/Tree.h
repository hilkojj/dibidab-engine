#pragma once

#include <math/math_utils.h>

#include <vector>
#include <string>
#include <memory>

namespace sol
{
    class state;
}

namespace dibidab::behavior
{
/**
 * Behavior trees can be created in C++ or Lua, or both.
 * Note that parent nodes should delete their children on their destruction.
 * The root node will be deleted by the BehaviorTree's destructor.
 * Created nodes that are not part of the tree will not be deleted automatically.
 */
    class Tree
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

            bool isAborting() const;

            virtual bool getEnteredDescription(std::vector<const char *> &descriptions) const;

            Node *setDescription(const char *description);

            std::string getReadableDebugInfo() const;

            virtual const char *getName() const = 0;

            virtual void drawDebugInfo() const
            {};

            bool hasFinishedAtLeastOnce() const;

            Result getLastResult() const;

            virtual ~Node() = default;

            struct
            {
                const char *path = nullptr;
                int line = 0;
            }
            source;

          protected:

            void registerAsParent(Node *child);

            virtual void onChildFinished(Node *child, Result result)
            {};

          private:
            Node *parent;
            bool bEntered;
            bool bAborting;

            std::string description;

            Result lastResult;
            bool bFinishedAtLeastOnce = false;

            friend class TreeInspector;
        };

        struct CompositeNode : public Node
        {
            void finish(Result result) override;

            // TODO: calling this in a running tree.
            virtual CompositeNode *addChild(Node *child);

            const std::vector<Node *> &getChildren() const;

            bool getEnteredDescription(std::vector<const char *> &descriptions) const override;

            ~CompositeNode() override;

            static constexpr int INVALID_CHILD_INDEX = -1;

          protected:
            void checkCorrectChildFinished(int expectedChildIndex, const Node *finishedChild) const;

          private:
            std::vector<Node *> children;
        };

        struct DecoratorNode : public Node
        {
            void enter() override;

            void abort() override;

            void finish(Result result) override;

            virtual DecoratorNode *setChild(Node *child);

            Node *getChild() const;

            bool getEnteredDescription(std::vector<const char *> &descriptions) const override;

            const char *getName() const override;

            ~DecoratorNode() override;

          protected:
            void onChildFinished(Node *child, Result result) override;

          private:
            Node *child = nullptr;
        };

        struct LeafNode : public Node
        {
        };

        // ------------------------ Basic Node classes: -------------------------- //

        struct SequenceNode : public CompositeNode
        {
            SequenceNode();

            void enter() override;

            void abort() override;

            void finish(Result result) override;

            const char *getName() const override;

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

            const char *getName() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;

          private:
            int currentChildIndex;
        };

        /**
         * Simple node that executes all children at the same time.
         * Finished when all children are finished. (TODO: add more options)
         * Will always return SUCCESS unless ABORTED, regardless of children results.
         */
        struct ParallelNode : public CompositeNode
        {
            ParallelNode();

            void enter() override;

            void abort() override;

            const char *getName() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;

          private:
            int numChildrenFinished;
        };

        struct InverterNode : public DecoratorNode
        {
            const char *getName() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;
        };

        struct SucceederNode : public DecoratorNode
        {
            void enter() override;

            const char *getName() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;
        };

        struct FailNode : public DecoratorNode
        {
            void enter() override;

            const char *getName() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;
        };

        struct RepeaterNode : public DecoratorNode
        {
            void enter() override;

            const char *getName() const override;

            void drawDebugInfo() const override;

          protected:
            void onChildFinished(Node *child, Result result) override;

          private:
            void enterChild();

#ifndef NDEBUG
            int timesRepeated = 0;
#endif
        };

        Tree();

        // Note: takes ownership of root
        void setRootNode(Node *root);

        Node *getRootNode() const;

      private:

        // Note: this is a shared pointer, so it only gets deleted when BehaviorTree is truly deleted and not copied.
        std::shared_ptr<Node> rootNode;

      public:
        static void addToLuaEnvironment(sol::state *lua);
    };
}
