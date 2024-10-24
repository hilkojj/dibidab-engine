#pragma once

#include <entt/entity/fwd.hpp>

#include <set>

struct loaded_asset;

namespace dibidab
{
    struct ComponentInfo;
    struct StructInfo;
    class StructInspector;
}

namespace dibidab::ecs
{
    class EntityEngine;
    class EntityTemplate;
    struct Inspecting;

    class EntityPicker
    {
      public:
        virtual entt::entity tryPick() = 0;

        virtual ~EntityPicker() = default;
    };


    class EntityMover
    {
      public:
        explicit EntityMover(entt::entity entity);

        virtual bool update() = 0;

        virtual ~EntityMover() = default;

      protected:
        const entt::entity entity;
    };


    class EntityInspector
    {
      public:
        explicit EntityInspector(EntityEngine &);

        void draw();

        ~EntityInspector();

      protected:
        virtual EntityPicker *createPicker();

        virtual EntityMover *createMover(entt::entity entityToMove);

        virtual const char *getUsedTemplateName(entt::entity entity) const;

        virtual const loaded_asset *getAssetForTemplate(const EntityTemplate &entityTemplate) const;

        virtual void editTemplateAsset(const loaded_asset &templateAsset);

      private:
        void updatePicking();

        void updateMoving();

        void drawInspectWindows();

        void drawInspectWindow(entt::entity entity, Inspecting &inspecting, bool &bKeepInspecting, bool &bDestroyEntity);

        void drawEntityNameField(entt::entity entity);

        const dibidab::ComponentInfo * drawComponentSelect(const std::set<const dibidab::ComponentInfo *> *exclude = nullptr) const;

        void drawAddComponents();

        void drawAddComponent(entt::entity entity);

        void drawMainMenuItem();

        void drawEntityTemplateSelect();

        void drawEntityRegistryTree();

        void drawInspectById();

        void drawCreateEntityFromTemplate();

        void drawBehaviorTreeInspectors();

        EntityTemplate *getUsedTemplate(entt::entity entity) const;

        std::set<const dibidab::ComponentInfo *> getComponentsForEntity(entt::entity entity) const;

        void addCustomDrawFunctions(StructInspector &structEditor);

      private:
        EntityEngine *engine;
        EntityPicker *picker = nullptr;
        EntityMover *mover = nullptr;

        struct CreatingEntity
        {
            EntityTemplate *fromTemplate = nullptr;
            bool bPersistent = false;
            bool bInspectOnCreate = false;
        }
        creatingEntity;
    };
}
