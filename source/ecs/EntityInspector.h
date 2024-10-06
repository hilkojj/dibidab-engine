
#ifndef GAME_ENTITYINSPECTOR_H
#define GAME_ENTITYINSPECTOR_H

#include "../../external/entt/src/entt/entity/registry.hpp"
#include "../macro_magic/component.h"
#include "../level/room/Room.h"
#include "entity_templates/LuaEntityTemplate.h"

#include <graphics/camera/camera.h>
#include <graphics/3d/renderers/debug_line_renderer.h>
#include <imgui.h>

struct Inspecting;

class EntityInspector
{
    std::string inspectorName;

    LuaEntityTemplate *creatingTempl = nullptr;
    json creatingTemplArgs;

  public:

    std::string createEntity_showSubFolder = "";
    bool createEntity_persistentOption = true;
    bool showInDropDown = true;

    bool
        pickEntity = false,
        moveEntity = false;

    entt::entity movingEntity = entt::null;

    EntityInspector(EntityEngine &, const std::string &name);

    void drawGUI(const Camera *cam, DebugLineRenderer &lineRenderer);

    static void drawInspectingDropDown();

  protected:
    EntityEngine &engine;
    entt::registry &reg;

    virtual void pickEntityGUI(const Camera *, DebugLineRenderer &);

    virtual void moveEntityGUI(const Camera *, DebugLineRenderer &);

    virtual void highLightEntity(entt::entity, const Camera *, DebugLineRenderer &);

  private:
    void createEntityGUI();

    void createEntity(const std::string &templateName);

    void templateArgsGUI();

    void editLuaScript(LuaEntityTemplate *);

    void drawNamedEntitiesTree(const Camera *, DebugLineRenderer &);

    void drawEntityInspectorGUI(entt::entity e, Inspecting &ins);

    void drawComponentFieldsTree(entt::entity e, Inspecting &ins, const char *componentName, const ComponentUtils *componentUtils);

    void drawAddComponent(entt::entity e, Inspecting &ins, const char *popupName);
};


#endif
