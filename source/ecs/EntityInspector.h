#pragma once

#include <entt/entity/fwd.hpp>

class EntityEngine;
class EntityTemplate;
struct Inspecting;
struct loaded_asset;


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

    void drawInspectWindow(const entt::entity entity, Inspecting &inspecting, bool &bKeepInspecting, bool &bDestroyEntity);

    void drawEntityNameField(const entt::entity entity);

    void drawMainMenuItem();

    void drawEntityTemplateSelect();

    void drawEntityRegistryTree();

    void drawInspectById();

    void drawCreateEntityFromTemplate();

    void drawBehaviorTreeInspectors();

    EntityTemplate *getUsedTemplate(entt::entity entity) const;

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
