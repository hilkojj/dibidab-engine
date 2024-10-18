
#include "EntityInspector.h"
#if 0
#include "entity_templates/LuaEntityTemplate.h"
#include "systems/EntitySystem.h"

#include "../ai/behavior_trees/BehaviorTreeInspector.h"

#include "../game/dibidab.h"

#include "components/LuaScripted.dibidab.h"
#include "components/Inspecting.dibidab.h"
#include "components/Children.dibidab.h"
#include "components/Brain.dibidab.h"

#include <gu/profiler.h>
#include <input/mouse_input.h>
#include <input/key_input.h>
#include <code_editor/CodeEditor.h>
#include <utils/string_utils.h>
#include <asset_manager/asset.h>
#include <asset_manager/AssetManager.h>
#include <files/file_utils.h>

#include <imgui_internal.h>
#endif

EntityInspector::EntityInspector(EntityEngine &engine, const std::string &name) : engine(engine), reg(engine.entities), inspectorName(name)
{

}

void EntityInspector::drawGUI(const Camera *cam, DebugLineRenderer &lineRenderer)
{

}

#if 0
std::list<std::string> inspectors;
std::string activeInspector;

static bool createEntityGUIJustOpened = false;

void EntityInspector::drawGUI(const Camera *cam, DebugLineRenderer &lineRenderer)
{
    gu::profiler::Zone z("entity inspector");

    if (showInDropDown)
        inspectors.push_front(inspectorName);
    if (pickEntity)
    {
        pickEntityGUI(cam, lineRenderer);
        return;
    }
    if (moveEntity)
    {
        moveEntityGUI(cam, lineRenderer);
        return;
    }
    if (activeInspector == inspectorName)
    {
        pickEntity = KeyInput::justPressed(dibidab::settings.keyInput.inspectEntity);
        moveEntity = KeyInput::justPressed(dibidab::settings.keyInput.moveEntity);

        if (KeyInput::justPressed(dibidab::settings.keyInput.createEntity))
        {
            ImGui::OpenPopup("Create entity");
            createEntityGUIJustOpened = true;
        }

        if (ImGui::BeginPopup("Create entity"))
        {
            createEntityGUI();
            ImGui::EndPopup();
        }
    }

    entt::entity addInspectingTo = entt::null;
    reg.view<Inspecting>().each([&](auto e, Inspecting &ins) {
        drawEntityInspectorGUI(e, ins);
        if (reg.valid(e) && reg.has<Inspecting>(e) && ins.addInspectingTo != entt::null)
        {
            addInspectingTo = ins.addInspectingTo;
            ins.addInspectingTo = entt::null;
        }
    });
    if (engine.entities.valid(addInspectingTo))
    {
        engine.entities.get_or_assign<Inspecting>(addInspectingTo);
    }

    if (creatingTempl)
        templateArgsGUI();

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu(inspectorName.c_str()))
    {
        auto str = std::to_string(reg.alive()) + " entities active";
        ImGui::MenuItem(str.c_str(), "", false, false);

        if (ImGui::BeginMenu("Create entity"))
        {
            createEntityGUI();
            ImGui::EndMenu();
        }

        pickEntity |= ImGui::MenuItem("Inspect entity", KeyInput::getKeyName(dibidab::settings.keyInput.inspectEntity));
        moveEntity |= ImGui::MenuItem("Move entity", KeyInput::getKeyName(dibidab::settings.keyInput.moveEntity));

        ImGui::SetNextWindowSize(ImVec2(490, 0));
        if (ImGui::BeginMenu("Entity tree"))
        {
            drawNamedEntitiesTree(cam, lineRenderer);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Systems"))
        {
            for (auto sys : engine.getSystems())
                ImGui::MenuItem(sys->name.c_str(), nullptr, &sys->bUpdatesEnabled);

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    engine.entities.view<InspectingBrain>(entt::exclude<BehaviorTreeInspector>).each([&] (entt::entity entity, auto)
    {
        engine.entities.assign<BehaviorTreeInspector>(entity, engine, entity);
    });

    engine.entities.view<BehaviorTreeInspector>().each([&] (entt::entity entity, BehaviorTreeInspector &inspector)
    {
        engine.entities.get_or_assign<InspectingBrain>(entity);
        if (!inspector.drawGUI())
        {
            engine.entities.remove<BehaviorTreeInspector>(entity);
            engine.entities.remove<InspectingBrain>(entity);
        }
    });
}

void EntityInspector::drawEntityInspectorGUI(entt::entity e, Inspecting &ins)
{
    if (!ins.show)
    {
        reg.remove<Inspecting>(e);
        return;
    }
    ImGui::SetNextWindowPos(ins.windowPos.x > 0 ? ins.windowPos : ImVec2(MouseInput::mouseX, MouseInput::mouseY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiCond_Once);

    std::string title(inspectorName + " Entity #" + std::to_string(int(e)));
    if (!ImGui::Begin(title.c_str(), &ins.show, ImGuiWindowFlags_NoSavedSettings))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    if (ImGui::Button("Destroy entity"))
    {
        reg.destroy(e);
        ImGui::End();
        return;
    }
    if (auto *luaScripted = reg.try_get<LuaScripted>(e))
    {
        if (luaScripted->usedTemplate != nullptr)
        {
            ImGui::SameLine();
            if (ImGui::Button("Regenerate"))
            {
                vec3 pos = engine.getPosition(e);

                bool persistent = reg.try_get<Persistent>(e) != nullptr;

                auto newE = luaScripted->usedTemplate->create(persistent);

                reg.get_or_assign<Inspecting>(newE).windowPos = ImGui::GetWindowPos();

                engine.setPosition(newE, pos);

                reg.destroy(e);
                ImGui::End();
                return;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("This will:\n- Destroy this entity\n- Create a new entity using the '%s' template\n- Copy the position from this entity", luaScripted->usedTemplate->name.c_str());
        }
    }
    if (reg.has<Brain>(e))
    {
        ImGui::SameLine();
        if (ImGui::Button("Inspect brain") && !reg.has<BehaviorTreeInspector>(e))
        {
            reg.assign<BehaviorTreeInspector>(e, engine, e);
        }
    }
    {
        std::string currName;
        if (auto name = engine.getName(e))
            currName = name;
        const int extraBuffer = 1024;
        char *ptr = new char[currName.length() + extraBuffer]();
        memcpy(ptr, &currName[0], currName.length());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputText("Name", ptr, currName.length() + extraBuffer))
            if (ImGui::IsItemDeactivatedAfterEdit())
                if (!engine.setName(e, ptr[0] == '\0' ? nullptr : ptr))
                    std::cerr << "Name '" << ptr << "' already in use!" << std::endl;
        delete[] ptr;
    }

    // ---- COMPONENTS TREE -------

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
    ImGui::Columns(2);
    ImGui::Separator();

    ImGui::Text("Components:");
    ImGui::NextColumn();
    std::string addComponentPopupName = "add_component_" + std::to_string(int(e));
    if (ImGui::Button("Add"))
        ImGui::OpenPopup(addComponentPopupName.c_str());
    drawAddComponent(e, ins, addComponentPopupName.c_str());

    ImGui::NextColumn();

    for (auto componentName : ComponentUtils::getAllComponentTypeNames())
    {
        auto componentUtils = ComponentUtils::getFor(componentName);
        if (!componentUtils->entityHasComponent(e, reg)) continue;

        ImGui::PushID(componentName.c_str());
        ImGui::AlignTextToFramePadding();

        bool dontRemove = true;
        bool component_open = ImGui::CollapsingHeader(componentName.c_str(), &dontRemove);
        if (!dontRemove)
        {
            componentUtils->removeComponent(e, reg);
            component_open = false;
        }
        ImGui::NextColumn();

        ImGui::NextColumn();
        if (component_open)
            drawComponentFieldsTree(e, ins, componentName.c_str(), componentUtils);

        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::PopStyleVar();

    ImGui::End();
}

void helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool drawStringInput(std::string &str, Inspecting &ins, bool expandable=true, const char *label="")
{
    bool &multiline = ins.getState().multiline;

    ImGui::AlignTextToFramePadding();

    if (expandable)
    {
        if (!multiline && ImGui::Button(" v "))
            multiline = true;
        else if (multiline && ImGui::Button(" ^ "))
            multiline  = false;
    }
    ImGui::SameLine();

    const int extraBuffer = 1024;
    char *ptr = new char[str.length() + extraBuffer]();
    memcpy(ptr, &str[0], str.length());
    if (
            ((multiline && expandable) && ImGui::InputTextMultiline(label, ptr, str.length() + extraBuffer))
            ||
            (!(multiline && expandable) && ImGui::InputText(label, ptr, str.length() + extraBuffer))
            )
    {
        str = std::string(ptr);
        return ImGui::IsItemDeactivatedAfterEdit();
    }
    delete[] ptr;
    return false;
}

bool drawJsonValue(json &value, Inspecting &ins, bool arrayPreview=true, bool readOnly=false)
{
    bool changed = false;
    if (readOnly || (arrayPreview && value.is_array()))
    {
        std::string str = value.dump();
        ImGui::Text("%s", str.c_str());
    }
    else if (value.is_number_float())
    {
        float v = value;
        if (changed = ImGui::DragFloat(" ", &v))
            value = v;
        ImGui::SameLine();
        helpMarker("Drag or double-click");
    }
    else if (value.is_boolean())
    {
        bool v = value;
        if (changed = ImGui::Checkbox(" ", &v))
            value = v;
    }
    else if (value.is_number_integer())
    {
        int v = value;
        if (changed = ImGui::DragInt(" ", &v))
            value = v;
        ImGui::SameLine();
        helpMarker("Drag or double-click");
    }
    else if (value.is_string())
    {
        std::string v = value;
        changed = drawStringInput(v, ins);
        value = v;
    }
    return changed;
}

bool drawJsonTree(json &obj, Inspecting &ins, bool editStructure=true, bool readOnlyValues=false)
{
    bool changed = false;
    int i = 0;
    int eraseI = -1;
    std::string eraseKey, addKey;
    json addJson;
    for (auto& [key, value] : obj.items()) { // also works for arrays: keys will be "0", "1", "2", etc.

        ins.currentPath.emplace_back(key);
        ImGui::PushID(i++);                      // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
        ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.

        std::string keyStr = key, keyLabel = obj.is_array() ? "[" + key + "]" : (editStructure ? "" : key);

        bool field_open = value.is_structured();
        if (field_open)
            field_open = ImGui::TreeNode("Object", "%s", keyLabel.c_str());
        else
            ImGui::TreeNodeEx(
                    "Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_None,
                    "%s", keyLabel.c_str()
            );

        if (editStructure)
        {
            ImGui::SameLine();
            if (ImGui::Button(" x "))
            {
                eraseI = i;
                eraseKey = key;
            }
            if (obj.is_object())
            {
                ImGui::SameLine();
                if (drawStringInput(keyStr, ins, false, "     "))
                {
                    eraseI = i;
                    eraseKey = key;
                    addKey = keyStr;
                    addJson = value;
                }
            }
        }

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();

        changed |= drawJsonValue(value, ins, true, readOnlyValues);

        ImGui::NextColumn();

        if (field_open)
        {
            changed |= drawJsonTree(value, ins, true, readOnlyValues);
            ImGui::TreePop();
        }
        ImGui::PopID();
        ins.currentPath.pop_back();
    }
    if (editStructure)
    {
        ImGui::Separator();
        ImGui::Text("Add: ");
        if (obj.is_array())
        {
            ImGui::NextColumn();
            if (changed |= ImGui::Button("float")) obj.push_back(float(0));
            ImGui::SameLine();
            if (changed |= ImGui::Button("int")) obj.push_back(int(0));
            ImGui::SameLine();
            if (changed |= ImGui::Button("string")) obj.push_back("");
            ImGui::SameLine();
            if (changed |= ImGui::Button("bool")) obj.push_back(bool(false));
            ImGui::SameLine();
            if (changed |= ImGui::Button("array")) obj.push_back(json::array());
            ImGui::SameLine();
            if (changed |= ImGui::Button("object")) obj.push_back(json::object());
            ImGui::SameLine();
            ImGui::NextColumn();
        } else
        {
            std::string &newKey = ins.getState().newKey;
            drawStringInput(newKey, ins, false, "  ");

            ImGui::NextColumn();
            if (changed |= ImGui::Button("float")) obj[newKey] = float(0);
            ImGui::SameLine();
            if (changed |= ImGui::Button("int")) obj[newKey] = int(0);
            ImGui::SameLine();
            if (changed |= ImGui::Button("string")) obj[newKey] = "";
            ImGui::SameLine();
            if (changed |= ImGui::Button("bool")) obj[newKey] = bool(false);
            ImGui::SameLine();
            if (changed |= ImGui::Button("array")) obj[newKey] = json::array();
            ImGui::SameLine();
            if (changed |= ImGui::Button("object")) obj[newKey] = json::object();
            ImGui::SameLine();
            ImGui::NextColumn();
        }
        ImGui::Separator();
    }
    if (eraseI > -1)
    {
        changed = true;
        if (obj.is_array()) obj.erase(eraseI - 1);
        else obj.erase(eraseKey);
    }
    if (!addKey.empty())
    {
        changed = true;
        obj[addKey] = addJson;
    }
    return changed;
}

bool drawFieldsTree(
    EntityEngine &engine,
    json &valuesArray, const SerializableStructInfo *info, Inspecting &ins,
    bool readOnly=false, bool forceEditReadOnly=false
)
{
    bool changed = false;
    for (int i = 0; i < info->nrOfFields; i++)
    {
        auto fieldName = info->fieldNames[i];
        const std::string fieldTypeName = info->fieldTypeNames[i];

        ImGui::PushID(fieldName);                      // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
        ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.

        json &value = valuesArray[i];

        bool field_open = value.is_structured();
        if (field_open)
            field_open = ImGui::TreeNode("Object", "%s", fieldName);
        else
            ImGui::TreeNodeEx(
                    "Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_None,
                    "%s", fieldName
            );

        ImGui::SameLine();
        ImGui::Text("%s", fieldTypeName.c_str());

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();

        auto subInfo = SerializableStructInfo::getFor(fieldTypeName.c_str());

        bool subReadOnly = !forceEditReadOnly && readOnly;

        if (fieldTypeName == "PersistentEntityRef")
        {
            PersistentEntityRef ref;
            ref = value;
            entt::entity resolvedEntity = entt::null;
            bool bResolved = ref.tryResolve(engine.entities, resolvedEntity);

            if (!bResolved)
            {
                ImGui::ButtonEx("Unresolved!", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
            }
            else if (resolvedEntity == entt::null)
            {
                ImGui::ButtonEx("entt::null", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
            }
            else if (!engine.entities.valid(resolvedEntity))
            {
                ImGui::ButtonEx("Invalid!", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
            }
            else
            {
                std::string btnTxt = "#" + std::to_string(int(resolvedEntity));
                if (ImGui::Button(btnTxt.c_str()))
                {
                    ins.addInspectingTo = resolvedEntity;
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Persistent Id: %ld", ref.getId());
            }
        }
        else
        {
            changed |= drawJsonValue(value, ins, !subInfo, subReadOnly);
        }

        ImGui::NextColumn();
        if (field_open)
        {
            ins.currentPath.emplace_back(fieldName);
            if (subInfo)
                changed |= drawFieldsTree(engine, value, subInfo, ins, subReadOnly);
            else changed |= drawJsonTree(
                    value, ins,
                    !info->structFieldIsFixedSize[i] && !subReadOnly,
                    subReadOnly
            );
            ins.currentPath.pop_back();
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    return changed;
}

void EntityInspector::drawComponentFieldsTree(entt::entity e, Inspecting &ins, const char *componentName, const ComponentUtils *componentUtils)
{
    auto info = SerializableStructInfo::getFor(componentName);
    json valuesArray;
    componentUtils->getJsonComponent(valuesArray, e, reg);

    assert(ins.currentPath.empty());
    ins.currentPath.emplace_back(componentName);
    if (drawFieldsTree(engine, valuesArray, info, ins))
    {
        try {
            componentUtils->setJsonComponent(valuesArray, e, reg);
        }
        catch (std::exception &e) {
            std::cerr << "Exception after editing component in inspector:\n" << e.what() << std::endl;
        }
    }
    ins.currentPath.pop_back();
}

void EntityInspector::drawAddComponent(entt::entity e, Inspecting &ins, const char *popupName)
{
    if (ImGui::BeginPopup(popupName))
    {
        ImGui::Text("Component Type:");
        ImGui::Separator();

        for (auto typeName : ComponentUtils::getAllComponentTypeNames())
        {
            auto utils = ComponentUtils::getFor(typeName);
            if (utils->entityHasComponent(e, reg))
                continue;
            if (ImGui::Selectable(typeName.c_str()))
            {
                ins.addingComponentTypeName = typeName;
                ins.addingComponentJson = utils->getDefaultJsonComponent();
            }
        }
        ImGui::EndPopup();
    }

    if (ins.addingComponentTypeName.empty()) return;

    ImGui::SetNextWindowPos(ImVec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    std::string title = "Adding " + ins.addingComponentTypeName + " to entity #" + std::to_string(int(e));
    bool open = true;
    ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("[Warning]:\nadding components (with wrong values) could crash the game!");
    ImGui::Columns(2);
    ImGui::Separator();

    drawFieldsTree(engine, ins.addingComponentJson, SerializableStructInfo::getFor(ins.addingComponentTypeName.c_str()), ins, false);

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::Button("Add"))
    {
        open = false;

        auto utils = ComponentUtils::getFor(ins.addingComponentTypeName);
        try
        {
            utils->setJsonComponent(ins.addingComponentJson, e, reg);
        } catch (std::exception& e) {
            std::cerr << "Exception while trying to add component in inspector:\n" << e.what() << std::endl;
        }
    }
    ImGui::End();
    if (!open)
        ins.addingComponentTypeName.clear();
}

bool createPersistent = false;

#define HOVERED_AND_PRESSED_ENTER (ImGui::IsItemHovered() && ImGui::IsKeyPressed(GLFW_KEY_ENTER))

void EntityInspector::createEntityGUI()
{
    static ImGuiTextFilter filter;
    if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        bool close = !filter.IsActive();
        filter = ImGuiTextFilter();
        if (close)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
    }

    if (ImGui::MenuItem("Empty entity"))
        reg.assign<Inspecting>(reg.create());

    ImGui::Separator();
    ImGui::MenuItem("From template:", nullptr, false, false);

    static bool _ = false;
    if (_)
    {
        ImGui::SetKeyboardFocusHere();
        _ = false;
    }
    if (createEntityGUIJustOpened)
    {
        _ = true;
        createEntityGUIJustOpened = false;
    }
    filter.Draw("Filter", 200);
    ImGui::NewLine();

    for (auto &templateName : engine.getTemplateNames())
    {
        if (!filter.PassFilter(templateName.c_str()))
            continue;

        auto name = templateName;
        const char *description = nullptr;

        auto templ = &engine.getTemplate(templateName);
        auto luaTempl = dynamic_cast<LuaEntityTemplate *>(templ);
        if (luaTempl)
        {
            name = su::split(luaTempl->script.getLoadedAsset()->shortPath, "scripts/entities/").back();
            if (!luaTempl->getDescription().empty())
                description = luaTempl->getDescription().c_str();
        }

        bool show = true;

        auto dirSplitted = su::split(name, "/");

        int subMenusOpened = 0;
        if (filter.Filters.empty()) for (int i = 0; i < dirSplitted.size() - 1; i++)
        {
            if (i == 0 && dirSplitted[0] == createEntity_showSubFolder)
                continue;

            ImGui::SetNextWindowContentSize(ImVec2(240, 0));
            if (!ImGui::BeginMenu(dirSplitted[i].c_str()))
            {
                show = false;
                break;
            }
            else subMenusOpened++;
        }

        if (show)
        {
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, 120);
            ImGui::SetColumnWidth(1, 120);

            bool create = false;

            if (ImGui::MenuItem(dirSplitted.back().c_str(), nullptr) || HOVERED_AND_PRESSED_ENTER)
            {
                create = true;
                createPersistent = false;
            }

            if (description && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", description);

            ImGui::NextColumn();
            if (!createEntity_persistentOption)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button(("Persistent###pers_" + name).c_str()) || HOVERED_AND_PRESSED_ENTER)
            {
                create = true;
                createPersistent = true;
            }
            if (!createEntity_persistentOption)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save the created entity to level file");

            if (luaTempl)
            {
                ImGui::SameLine();
                auto str = "Edit###edit_" + name;
                if (ImGui::Button(str.c_str()) || HOVERED_AND_PRESSED_ENTER)
                    editLuaScript(luaTempl);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Edit the Lua script for this entity");
            }

            ImGui::Columns(1);

            if (create)
                createEntity(dirSplitted.back());
        }

        for (int i = 0; i < subMenusOpened; i++)
            ImGui::EndMenu();
    }
}

void EntityInspector::createEntity(const std::string &templateName)
{
    auto templ = &engine.getTemplate(templateName);

    auto *luaTempl = dynamic_cast<LuaEntityTemplate *>(templ);

    if (luaTempl && !luaTempl->getDefaultArgs().empty())
    {
        creatingTempl = luaTempl;
        creatingTemplArgs = luaTempl->getDefaultArgs();
    }
    else
    {
        moveEntity = true;
        movingEntity = templ->create(createPersistent);
    }
}

void EntityInspector::templateArgsGUI()
{
    ImGui::SetNextWindowPos(ImVec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    const std::string &name = creatingTempl->script.getLoadedAsset()->shortPath;
    std::string title = "Creating " + name;
    bool open = !ImGui::IsKeyPressed(GLFW_KEY_ESCAPE);
    ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Columns(2);
    ImGui::Separator();

    static Inspecting ins;
    drawJsonTree(creatingTemplArgs, ins, false);

    ImGui::Columns(1);
    ImGui::Separator();

    if (createEntity_persistentOption)
    {
        ImGui::Checkbox("Persistent", &createPersistent);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save the created entity to level file");
    }

    if (ImGui::Button("Create") || HOVERED_AND_PRESSED_ENTER)
    {
        open = false;
        auto e = reg.create();
        creatingTempl->createComponentsWithJsonArguments(e, creatingTemplArgs, createPersistent);
        moveEntity = true;
        movingEntity = e;
    }
    ImGui::SetItemDefaultFocus();

    ImGui::End();
    if (!open)
    {
        creatingTempl = nullptr;
        creatingTemplArgs.clear();
    }
}

void EntityInspector::editLuaScript(LuaEntityTemplate *luaTemplate)
{
    auto script = luaTemplate->script;
    for (auto &t : CodeEditor::tabs)
        if (t.title == script.getLoadedAsset()->fullPath)
            return;

    auto &tab = CodeEditor::tabs.emplace_back();
    tab.title = script.getLoadedAsset()->fullPath;
    tab.code = fu::readString(script.getLoadedAsset()->fullPath.c_str());
    tab.languageDefinition = TextEditor::LanguageDefinition::C(); // the lua definition is pretty broken
    tab.save = [script] (auto &tab) {

        fu::writeBinary(script.getLoadedAsset()->fullPath.c_str(), tab.code.data(), tab.code.length()); // todo: why is this called writeBINARY? lol

        AssetManager::loadFile(script.getLoadedAsset()->fullPath, "assets/");
    };
    tab.revert = [script] (auto &tab) {
        tab.code = fu::readString(script.getLoadedAsset()->fullPath.c_str());
    };
}

void EntityInspector::drawInspectingDropDown()
{
    if (inspectors.empty())
        return;

    if (inspectors.size() == 1)
    {
        activeInspector = inspectors.front();
        inspectors.clear();
        return;
    }

    ImGui::Text("    Inspecting:");
    ImGui::SetNextItemWidth(120);

    auto prevInspector = activeInspector;
    activeInspector.clear();

    bool draw = ImGui::BeginCombo("    ", prevInspector.c_str());
    for (auto &name : inspectors)
    {
        if (activeInspector.empty() || prevInspector == name)
            activeInspector = name;

        if (draw && ImGui::Selectable(name.c_str()))
        {
            activeInspector = name;
            break;
        }
    }

    if (draw)
        ImGui::EndCombo();

    inspectors.clear();
}

void EntityInspector::pickEntityGUI(const Camera *cam, DebugLineRenderer &lineRenderer)
{
    pickEntity = false;
}

void EntityInspector::moveEntityGUI(const Camera *cam, DebugLineRenderer &lineRenderer)
{
    moveEntity = false;
}

void EntityInspector::highLightEntity(entt::entity, const Camera *, DebugLineRenderer &)
{
    // :)
}

void EntityInspector::drawNamedEntitiesTree(const Camera *cam, DebugLineRenderer &lineRenderer)
{
    {
        static int inspectByID = 0;
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Inspect by ID", &inspectByID);
        ImGui::SameLine();
        bool valid = reg.valid(entt::entity(inspectByID));
        bool inspect = ImGui::Button("Inspect");
        if (ImGui::IsItemHovered() && !valid)
            ImGui::SetTooltip("Entity #%d does not exist!", int(inspectByID));
        if (inspect && valid)
            engine.entities.assign_or_replace<Inspecting>(entt::entity(inspectByID));
    }

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 400);
    ImGui::Separator();

    struct funcs
    {
        static void showEntity(const std::string &name, entt::entity e, EntityEngine &engine)
        {
            ImGui::PushID(&name);                      // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
            ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.

            Parent *isParent = engine.entities.try_get<Parent>(e);
            bool node_open = false;

            if (!isParent)
                ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_None);
            else
                node_open = ImGui::TreeNode(name.c_str());

            LuaScripted *luaScripted = engine.entities.try_get<LuaScripted>(e);
            if (luaScripted && luaScripted->usedTemplate)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("Lua: %s", luaScripted->usedTemplate->name.c_str());
            }

            ImGui::NextColumn();
            ImGui::AlignTextToFramePadding();
            if (ImGui::Button(std::to_string(int(e)).c_str()))
                engine.entities.assign_or_replace<Inspecting>(e);
            ImGui::NextColumn();
            if (node_open && isParent)
            {
                for (auto &[childName, child] : isParent->childrenByName)
                {
                    std::string childNameStr;
                    if (auto globalName = engine.getName(child))
                    {
                        childNameStr = globalName;
                        childNameStr += " ";
                    }
                    childNameStr += "[child '" + childName + "']";
                    funcs::showEntity(childNameStr, child, engine);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    };

    for (auto &[name, e] : engine.getNamedEntities())
        funcs::showEntity(name, e, engine);

    ImGui::Columns(1);
}
#endif
