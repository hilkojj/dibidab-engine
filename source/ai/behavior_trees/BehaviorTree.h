
#ifndef GAME_BEHAVIORTREE_H
#define GAME_BEHAVIORTREE_H

#include "../../ecs/EntityEngine.h"

#include "../../luau.h"

#include <vector>

/**
 * Behavior trees can be created in C++ or Lua, or both.
 * Note that parent nodes should delete their children on their destruction.
 * The root node will be deleted by the BehaviorTree's destructor.
 * Created nodes that are not part of the tree will not be deleted automatically.
 */
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

        bool hasLuaDebugInfo() const;

        const lua_Debug &getLuaDebugInfo() const;

        virtual const char *getName() const = 0;

        virtual void drawDebugInfo() const {};

#ifndef NDEBUG
        bool hasFinishedAtLeastOnce() const;

        Result getLastResult() const;
#endif

        virtual ~Node() = default;

      protected:

        void registerAsParent(Node *child);

        virtual void onChildFinished(Node *child, Result result) {};

      private:
        Node *parent;
        bool bEntered;
        bool bAborted;

        lua_Debug luaDebugInfo;
        bool bHasLuaDebugInfo;

#ifndef NDEBUG
        Result lastResult;
        bool bFinishedAtLeastOnce = false;
#endif
        friend class BehaviorTreeInspector;
    };

    struct CompositeNode : public Node
    {
        void finish(Result result) override;

        // TODO: calling this in a running tree.
        virtual CompositeNode *addChild(Node *child);

        const std::vector<Node *> &getChildren() const;

        ~CompositeNode() override;

      private:
        std::vector<Node *> children;
    };

    struct DecoratorNode : public Node
    {
        void abort() override;

        void finish(Result result) override;

        virtual DecoratorNode *setChild(Node *child);

        Node *getChild() const;

        ~DecoratorNode() override;

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

    struct InverterNode : public DecoratorNode
    {
        void enter() override;

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

    struct WaitNode : public LeafNode
    {
        WaitNode* finishAfter(float seconds, entt::entity waitingEntity, EntityEngine *engine);

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        const char *getName() const override;

        void drawDebugInfo() const override;

      private:
        /**
         * If set to >= 0, will abort in n frames (where n could be 0, in case `seconds` <= deltaTime).
         * Else, will only abort if parent aborts.
         */
        float seconds = -1.0f;
        /**
         * Can be any entity. If the entity gets destroyed while waiting, abort() will only be called if the parent aborts.
         */
        entt::entity waitingEntity = entt::null;
        EntityEngine *engine = nullptr;

        delegate_method onWaitFinished;

#ifndef NDEBUG
        float timeStarted = 0.0f;
#endif
    };

    // ------------------------ Event-based Node classes: -------------------------- //

    struct ComponentObserverNode : public CompositeNode
    {
        ComponentObserverNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        template<class Component>
        ComponentObserverNode *has(EntityEngine *engine, entt::entity entity)
        {
            has(engine, entity, ComponentUtils::getFor<Component>());
            return this;
        }

        void has(EntityEngine *engine, entt::entity entity, const ComponentUtils *componentUtils);

        template<class Component>
        ComponentObserverNode *exclude(EntityEngine *engine, entt::entity entity)
        {
            exclude(engine, entity, ComponentUtils::getFor<Component>());
            return this;
        }

        void exclude(EntityEngine *engine, entt::entity entity, const ComponentUtils *componentUtils);

        ComponentObserverNode *setOnFulfilledNode(Node *child);

        ComponentObserverNode *setOnUnfulfilledNode(Node *child);

        // Do not use:
        CompositeNode *addChild(Node *child) override;

        const char *getName() const override;

        void drawDebugInfo() const override;

        ~ComponentObserverNode() override;

      protected:
        void onChildFinished(Node *child, Result result) override;

      private:

        void observe(EntityEngine *engine, entt::entity entity, const ComponentUtils *componentUtils, bool presentValue,
            bool absentValue);

        bool allConditionsFulfilled() const;

        void onConditionsChanged(EntityEngine *engine, entt::entity entity);

        int getChildIndexToEnter() const;

        void enterChild();

        struct ObserverHandle
        {
            EntityEngine *engine = nullptr;
            const ComponentUtils *componentUtils = nullptr;
            EntityObserver::Handle handle;
        };

        std::vector<ObserverHandle> observerHandles;
        std::vector<bool> conditions;
        int fulfilledNodeIndex;
        int unfulfilledNodeIndex;
        bool bFulFilled;
        int currentNodeIndex;

        friend class BehaviorTreeInspector;
    };

    // ------------------------ Customization Node classes: -------------------------- //

    struct LuaLeafNode : public LeafNode
    {
        void enter() override;

        void abort() override;

        void finish(Result result) override;

        const char *getName() const override;

        sol::function luaEnterFunction;
        sol::function luaAbortFunction;

      private:
        void finishAborted();

        bool bInEnterFunction = false;
    };

    struct FunctionalLeafNode : public LeafNode
    {
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

        bool bInEnterFunction = false;
    };

    BehaviorTree();

    void setRootNode(Node *root);

    Node *getRootNode() const;

  private:

    // Note: this is a shared pointer, so it only gets deleted when BehaviorTree is truly deleted and not copied.
    std::shared_ptr<Node> rootNode;

  public:
    static void addToLuaEnvironment(sol::state *lua);
};


#endif //GAME_BEHAVIORTREE_H
