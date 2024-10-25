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
    class Engine;
    class Template;
    struct Inspecting;

    class Picker
    {
      public:
        virtual entt::entity tryPick() = 0;

        virtual ~Picker() = default;
    };


    class Mover
    {
      public:
        explicit Mover(entt::entity entity);

        virtual bool update() = 0;

        virtual ~Mover() = default;

      protected:
        const entt::entity entity;
    };


    class Inspector
    {
      public:
        explicit Inspector(Engine &);

        void draw();

        ~Inspector();

      protected:
        virtual Picker *createPicker();

        virtual Mover *createMover(entt::entity entityToMove);

        virtual const char *getUsedTemplateName(entt::entity entity) const;

        virtual const loaded_asset *getAssetForTemplate(const Template &entityTemplate) const;

        virtual void editTemplateAsset(const loaded_asset &templateAsset);

      private:
        void updatePicking();

        void updateMoving();

        void drawInspectWindows();

        void drawInspectWindow(entt::entity entity, Inspecting &inspecting, bool &bKeepInspecting, bool &bDestroyEntity);

        void drawEntityNameField(entt::entity entity);

        const ComponentInfo *drawComponentSelect(const std::set<const ComponentInfo *> *exclude = nullptr) const;

        void drawAddComponents();

        void drawAddComponent(entt::entity entity);

        void drawMainMenuItem();

        void drawEntityTemplateSelect();

        void drawEntityRegistryTree();

        void drawInspectById();

        void drawCreateEntityFromTemplate();

        void drawBehaviorTreeInspectors();

        Template *getUsedTemplate(entt::entity entity) const;

        std::set<const ComponentInfo *> getComponentsForEntity(entt::entity entity) const;

        void addCustomDrawFunctions(StructInspector &structEditor);

        Engine *engine;
        Picker *picker = nullptr;
        Mover *mover = nullptr;

        struct CreatingEntity
        {
            Template *fromTemplate = nullptr;
            bool bPersistent = false;
            bool bInspectOnCreate = false;
        }
        creatingEntity;
    };
}
