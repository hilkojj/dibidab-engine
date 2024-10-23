#pragma once

#include <entt/entity/fwd.hpp>

#include <set>

class EntityEngine;
class EntityTemplate;
class StructEditor;
struct Inspecting;
struct loaded_asset;

namespace dibidab
{
    struct component_info;
    struct struct_info;
}


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

    const dibidab::component_info *drawComponentSelect(const std::set<const dibidab::component_info *> *exclude = nullptr) const;

    void drawAddComponents();

    void drawAddComponent(entt::entity entity);

    void drawMainMenuItem();

    void drawEntityTemplateSelect();

    void drawEntityRegistryTree();

    void drawInspectById();

    void drawCreateEntityFromTemplate();

    void drawBehaviorTreeInspectors();

    EntityTemplate *getUsedTemplate(entt::entity entity) const;

    std::set<const dibidab::component_info *> getComponentsForEntity(entt::entity entity) const;

    void addCustomDrawFunctions(StructEditor &structEditor);

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
