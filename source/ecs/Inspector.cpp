#include "Inspector.h"

#include "Engine.h"

#include "systems/System.h"
#include "components/Inspecting.dibidab.h"
#include "components/LuaScripted.dibidab.h"
#include "components/Children.dibidab.h"
#include "components/Brain.dibidab.h"
#include "templates/TemplateArgs.dibidab.h"
#include "templates/LuaTemplate.h"

#include "../behavior/TreeInspector.h"
#include "../dibidab/dibidab.h"
#include "../reflection/StructInfo.h"
#include "../reflection/ComponentInfo.h"

#include "../generated/registry.struct_json.h"

#include <assets/AssetManager.h>
#include <input/mouse_input.h>
#include <utils/string_utils.h>
#include <code_editor/CodeEditor.h>
#include <files/file_utils.h>
#include <gu/profiler.h>

#include <json.hpp>
#include <imgui_internal.h>

namespace dibidab::ecs
{
    struct AddingComponent
    {
        const dibidab::ComponentInfo *componentInfo = nullptr;
        json componentJson;
        StructInspector editor;
    };
}

dibidab::ecs::Mover::Mover(Engine &engine, entt::entity entity) :
    engine(&engine),
    entity(entity)
{
}

dibidab::ecs::Inspector::Inspector(Engine &engine) :
    engine(&engine)
{
}

void dibidab::ecs::Inspector::draw()
{
    gu::profiler::Zone profilerZone("EntityInspector");

    if (picker != nullptr)
    {
        updatePicking();
    }
    else if (KeyInput::justPressed(dibidab::settings.developerKeyInput.inspectEntity))
    {
        picker = createPicker();
    }
    else if (mover != nullptr)
    {
        updateMoving();
    }
    else
    {
        drawInspectWindows();
    }

    if (KeyInput::justPressed(dibidab::settings.developerKeyInput.createEntity))
    {
        ImGui::OpenPopup("Create entity");
    }
    if (ImGui::BeginPopup("Create entity"))
    {
        drawEntityTemplateSelect();
        ImGui::EndPopup();
    }

    if (creatingEntity.fromTemplate != nullptr)
    {
        drawCreateEntityFromTemplate();
    }

    drawMainMenuItem();
    drawAddComponents();
    drawBehaviorTreeInspectors();
}

dibidab::ecs::Inspector::~Inspector()
{
    delete picker;
    delete mover;
}

dibidab::ecs::Picker *dibidab::ecs::Inspector::createPicker()
{
    return nullptr;
}

dibidab::ecs::Mover *dibidab::ecs::Inspector::createMover(const entt::entity entityToMove)
{
    return nullptr;
}

const char *dibidab::ecs::Inspector::getUsedTemplateName(entt::entity entity) const
{
    if (const LuaScripted *scripted = engine->entities.try_get<LuaScripted>(entity))
    {
        if (scripted->usedTemplate != nullptr)
        {
            return scripted->usedTemplate->name.c_str();
        }
    }
    return nullptr;
}

const loaded_asset *dibidab::ecs::Inspector::getAssetForTemplate(const Template &entityTemplate) const
{
    if (const LuaTemplate *luaEntityTemplate = dynamic_cast<const LuaTemplate *>(&entityTemplate))
    {
        return luaEntityTemplate->script.getLoadedAsset();
    }
    return nullptr;
}

void dibidab::ecs::Inspector::editTemplateAsset(const loaded_asset &templateAsset)
{
    CodeEditor::Tab &codeTab = CodeEditor::tabs.emplace_back();
    codeTab.title = templateAsset.fullPath;
    codeTab.code = fu::readString(templateAsset.fullPath.c_str());
    codeTab.languageDefinition = TextEditor::LanguageDefinition::C(); // C syntax highlighting works better than Lua
    codeTab.save = [&templateAsset] (const CodeEditor::Tab &tab)
    {
        fu::writeBinary(templateAsset.fullPath.c_str(), tab.code.c_str(), tab.code.length());
        AssetManager::loadFile(templateAsset.fullPath, "assets/");
    };
    codeTab.revert = [&templateAsset] (CodeEditor::Tab &tab)
    {
        tab.code = fu::readString(templateAsset.fullPath.c_str());
    };
}

void dibidab::ecs::Inspector::updatePicking()
{
    const entt::entity pickedEntity = picker->tryPick();
    if (engine->entities.valid(pickedEntity))
    {
        Inspecting &inspecting = engine->entities.assign_or_replace<Inspecting>(pickedEntity);
        inspecting.nextWindowPos.emplace(MouseInput::mouseX, MouseInput::mouseY);
        delete picker;
        picker = nullptr;
    }
    if (KeyInput::justPressed(dibidab::settings.developerKeyInput.cancel))
    {
        delete picker;
        picker = nullptr;
    }
}

void dibidab::ecs::Inspector::updateMoving()
{
    if (!mover->update())
    {
        delete mover;
        mover = nullptr;
    }
}

void dibidab::ecs::Inspector::drawInspectWindows()
{
    std::vector<entt::entity> toInspect;
    engine->entities.view<Inspecting>().each([&] (const entt::entity e, Inspecting &inspecting)
    {
        toInspect.push_back(e);
    });

    for (const entt::entity entity : toInspect)
    {
        if (!engine->entities.valid(entity))
        {
            continue;
        }
        if (Inspecting *inspecting = engine->entities.try_get<Inspecting>(entity))
        {
            bool bKeepInspecting = true;
            bool bDestroyEntity = false;
            drawInspectWindow(entity, *inspecting, bKeepInspecting, bDestroyEntity);
            if (!bKeepInspecting)
            {
                engine->entities.remove<Inspecting>(entity);
            }
            if (bDestroyEntity)
            {
                engine->entities.destroy(entity);
            }
        }
    }
}

void dibidab::ecs::Inspector::drawInspectWindow(const entt::entity entity, Inspecting &inspecting, bool &bKeepInspecting, bool &bDestroyEntity)
{
    if (inspecting.nextWindowPos.has_value())
    {
        ImGui::SetNextWindowPos(inspecting.nextWindowPos.value());
        inspecting.nextWindowPos.reset();
    }
    ImGui::SetNextWindowSize(vec2(500, 680), ImGuiCond_Once);

    std::string inspectWindowTile = "Inspect #" + std::to_string(int(entity));
    if (const char *templateName = getUsedTemplateName(entity))
    {
        inspectWindowTile += " (";
        inspectWindowTile += templateName;
        inspectWindowTile += ")";
    }
    if (!ImGui::Begin(inspectWindowTile.c_str(), &bKeepInspecting, ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }
    ImGui::BeginChild("EntityOptions", vec2(-1, 36), true);

    drawEntityNameField(entity);

    ImGui::SameLine();
    if (ImGui::Button("Move"))
    {
        delete mover;
        mover = createMover(entity);
    }

    ImGui::SameLine();
    bDestroyEntity = ImGui::Button("Destroy");

    if (Template *usedTemplate = getUsedTemplate(entity))
    {
        ImGui::SameLine();
        if (ImGui::Button("Regenerate"))
        {
            bDestroyEntity = true;
            const entt::entity newEntity = usedTemplate->create(engine->entities.has<Persistent>(entity));
            engine->entities.assign_or_replace<Inspecting>(newEntity).nextWindowPos = ImGui::GetWindowPos();
        }
    }

    if (engine->entities.has<Brain>(entity))
    {
        ImGui::SameLine();
        if (ImGui::Button("Inspect brain") && !engine->entities.has<behavior::TreeInspector>(entity))
        {
            engine->entities.assign<behavior::TreeInspector>(entity, *engine, entity);
        }
    }
    ImGui::EndChild();

    const std::set<const dibidab::ComponentInfo *> ownedComponents = getComponentsForEntity(entity);

    ImGui::Columns(2);

    ImGui::Text("Components:");
    ImGui::NextColumn();
    if (ImGui::Button("Add"))
    {
        ImGui::OpenPopup("AddComponent");
    }
    ImGui::NextColumn();
    ImGui::Columns(1);

    if (ImGui::BeginPopup("AddComponent"))
    {
        if (const dibidab::ComponentInfo *info = drawComponentSelect(&ownedComponents))
        {
            if (const dibidab::StructInfo *structInfo = dibidab::findStructInfo(info->structId))
            {
                AddingComponent &adding = engine->entities.assign_or_replace<AddingComponent>(entity, AddingComponent {
                    info,
                    structInfo->getDefaultJsonObject ? structInfo->getDefaultJsonObject() : json::object(),
                    StructInspector(*structInfo)
                });
                addCustomDrawFunctions(adding.editor);
            }
        }
        ImGui::EndPopup();
    }

    for (const dibidab::ComponentInfo *component : ownedComponents)
    {
        if (component->structId == typename_utils::getTypeName<Inspecting>(false))
        {
            // Do not draw the component that makes this window appear.
            // Note that it would allow for deleting the component, to which we have a reference here: `inspecting`
            continue;
        }
        bool bKeepComponent = true;
        bool bComponentOpened = ImGui::CollapsingHeader(component->name, &bKeepComponent);
        if (ImGui::IsItemHovered() && strcmp(component->name, component->structId) != 0)
        {
            ImGui::SetTooltip("%s", component->structId);
        }
        if (bComponentOpened)
        {
            if (const dibidab::StructInfo *structInfo = dibidab::findStructInfo(component->structId))
            {
                if (inspecting.componentInspectors.find(component) == inspecting.componentInspectors.end())
                {
                    inspecting.componentInspectors.insert({ component, StructInspector(*structInfo) });
                    addCustomDrawFunctions(inspecting.componentInspectors.at(component));
                }
                StructInspector &editor = inspecting.componentInspectors.at(component);
                json componentJson;
                if (component->getJsonObject)
                {
                    component->getJsonObject(entity, engine->entities, componentJson);
                }
                if (editor.draw(componentJson))
                {
                    try
                    {
                        if (component->setFromJson)
                        {
                            component->setFromJson(componentJson, entity, engine->entities);
                        }
                    }
                    catch (const nlohmann::detail::exception &exception)
                    {
                        editor.jsonParseError = exception.what();
                    }
                }
            }
        }
        if (!bKeepComponent)
        {
            component->removeComponent(entity, engine->entities);
        }
    }

    ImGui::End();
}

void dibidab::ecs::Inspector::drawEntityNameField(const entt::entity entity)
{
    std::string currentName;
    if (const char *name = engine->getName(entity))
    {
        currentName = name;
    }
    const int extraBuffer = 128;
    char *buffer = new char[currentName.length() + extraBuffer]();
    memcpy(buffer, &currentName[0], currentName.length());
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputText("Name", buffer, currentName.length() + extraBuffer)
        && ImGui::IsItemDeactivatedAfterEdit())
    {
        engine->setName(entity, buffer[0] == '\0' ? nullptr : buffer);
    }
    delete[] buffer;
}

const dibidab::ComponentInfo *dibidab::ecs::Inspector::drawComponentSelect(const std::set<const dibidab::ComponentInfo *> *exclude) const
{
    static ImGuiTextFilter filter;
    if (ImGui::IsWindowAppearing())
    {
        filter.Clear();
    }
    filter.Draw();

    const dibidab::ComponentInfo *componentToReturn = nullptr;

    std::vector<std::string> commonCategoryPath;
    for (const auto &[name, info] : dibidab::getAllComponentInfos())
    {
        int i = 0;
        for (const char *directory : info.categoryPath)
        {
            if (commonCategoryPath.size() <= i)
            {
                commonCategoryPath.push_back(directory);
            }
            ++i;
        }
    }
    for (const auto &[name, info] : dibidab::getAllComponentInfos())
    {
        int i = 0;
        for (const char *directory : info.categoryPath)
        {
            if (commonCategoryPath.size() > i && commonCategoryPath[i] != directory)
            {
                commonCategoryPath.resize(i);
                break;
            }
            ++i;
        }
        if (info.categoryPath.size() < commonCategoryPath.size())
        {
            commonCategoryPath.resize(info.categoryPath.size());
        }
    }

    for (const auto &[name, info] : dibidab::getAllComponentInfos())
    {
        if (!filter.PassFilter(info.name))
        {
            continue;
        }
        if (exclude != nullptr && exclude->find(&info) != exclude->end())
        {
            continue;
        }
        int categoriesOpened = 0;
        if (!filter.IsActive())
        {
            for (int i = commonCategoryPath.size(); i < info.categoryPath.size(); i++)
            {
                if (ImGui::BeginMenu(info.categoryPath[i]))
                {
                    ++categoriesOpened;
                }
                else
                {
                    break;
                }
            }
        }
        if ((filter.IsActive() || categoriesOpened == info.categoryPath.size() - commonCategoryPath.size())
            && ImGui::MenuItem(info.name))
        {
            componentToReturn = &info;
        }
        for (int i = 0; i < categoriesOpened; i++)
        {
            ImGui::EndMenu();
        }
    }
    return componentToReturn;
}

void dibidab::ecs::Inspector::drawAddComponents()
{
    engine->entities.view<AddingComponent>().each([&] (entt::entity e, AddingComponent &)
    {
        drawAddComponent(e);
    });
}

void dibidab::ecs::Inspector::drawAddComponent(const entt::entity entity)
{
    AddingComponent &addingComponent = engine->entities.get<AddingComponent>(entity);

    const std::string title = std::string("Adding ") + addingComponent.componentInfo->name;
    bool bOpen = true;

    ImGui::SetNextWindowPos(ImVec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    if (ImGui::Begin(title.c_str(), &bOpen, ImGuiWindowFlags_NoSavedSettings))
    {
        addingComponent.editor.draw(addingComponent.componentJson);

        ImGui::Separator();
        if (ImGui::Button("Add"))
        {
            try
            {
                if (addingComponent.componentInfo->setFromJson)
                {
                    addingComponent.componentInfo->setFromJson(addingComponent.componentJson, entity, engine->entities);
                }
                else
                {
                    addingComponent.componentInfo->addComponent(entity, engine->entities);
                }
                bOpen = false;
            }
            catch (const nlohmann::detail::exception &e)
            {
                addingComponent.editor.jsonParseError = e.what();
            }
        }
    }
    ImGui::End();

    if (!bOpen)
    {
        engine->entities.remove<AddingComponent>(entity);
    }
}

void dibidab::ecs::Inspector::drawMainMenuItem()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Inspect"))
    {
        const std::string entitiesAliveStr = std::to_string(engine->entities.alive()) + " Entities alive";
        ImGui::MenuItem(entitiesAliveStr.c_str(), nullptr, false, false);

        if (ImGui::BeginMenu("Create Entity"))
        {
            drawEntityTemplateSelect();
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Pick to Inspect", KeyInput::getKeyName(dibidab::settings.developerKeyInput.inspectEntity)))
        {
            delete picker;
            picker = createPicker();
        }

        if (ImGui::BeginMenu("Entity Tree"))
        {
            drawEntityRegistryTree();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Enabled Systems"))
        {
            for (const System *system : engine->getSystems())
            {
                ImGui::MenuItem(system->name.c_str(), nullptr, system->bUpdatesEnabled);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void dibidab::ecs::Inspector::drawEntityTemplateSelect()
{
    static ImGuiTextFilter templateFilter;
    if (ImGui::IsWindowAppearing())
    {
        templateFilter.Clear();
    }

    if (ImGui::MenuItem("Empty Entity"))
    {
        engine->entities.assign<Inspecting>(engine->entities.create());
    }

    ImGui::Separator();
    ImGui::TextDisabled("From Template:");

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }
    templateFilter.Draw();

    for (const std::string &templateName : engine->getTemplateNames())
    {
        if (!templateFilter.PassFilter(templateName.c_str()))
        {
            continue;
        }
        Template &entityTemplate = engine->getTemplate(templateName);
        const loaded_asset *loadedAsset = getAssetForTemplate(entityTemplate);

        std::vector<std::string> categoryPath;

        // Get category path from loaded asset path, only if we are not filtering. Show flat list otherwise.
        if (!templateFilter.IsActive() && loadedAsset != nullptr)
        {
            categoryPath = su::split(su::split(loadedAsset->shortPath, engine->getTemplateDirectoryPath()).back(), "/");
            categoryPath.pop_back();
        }

        int categoriesOpened = 0;
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        for (const std::string &category : categoryPath)
        {
            if (ImGui::BeginMenu(category.c_str()))
            {
                categoriesOpened++;
            }
            else
            {
                break;
            }
        }
        ImGui::PopStyleColor();

        const bool bShowTemplateItem = categoriesOpened == categoryPath.size();

        if (bShowTemplateItem && ImGui::BeginMenu(templateName.c_str()))
        {
            if (!entityTemplate.getDescription().empty())
            {
                ImGui::TextDisabled("%s", entityTemplate.getDescription().c_str());
                ImGui::Separator();
            }
            if (ImGui::MenuItem("Create"))
            {
                creatingEntity = {
                    &entityTemplate,
                    false
                };
            }
            if (ImGui::MenuItem("Create Persistent"))
            {
                creatingEntity = {
                    &entityTemplate,
                    true
                };
            }
            if (loadedAsset != nullptr && ImGui::MenuItem("Edit Script"))
            {
                editTemplateAsset(*loadedAsset);
            }
            ImGui::EndMenu();
        }

        for (int i = 0; i < categoriesOpened; i++)
        {
            ImGui::EndMenu();
        }
    }
}


void dibidab::ecs::Inspector::drawEntityRegistryTree()
{
    drawInspectById();

    ImGui::Separator();

    static ImGuiTextFilter nameFilter;
    static ImGuiTextFilter templateFilter;
    if (ImGui::IsWindowAppearing())
    {
        nameFilter.Clear();
        templateFilter.Clear();
    }
    ImGui::TextDisabled("Filter (inc,-exc):");
    nameFilter.Draw("Name");
    templateFilter.Draw("Template");

    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, 400);

    std::function<void(entt::entity, const std::string &name)> drawEntity;

    drawEntity = [&] (const entt::entity entity, const std::string &name)
    {
        ImGui::PushID(name.c_str());
        const Parent *parentComponent = engine->entities.try_get<Parent>(entity);
        const bool bIsParent = parentComponent != nullptr;
        bool bShowChildren = false;
        bool bWasShowingChildren = ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(name.c_str()));

        if (bIsParent)
        {
            bShowChildren = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow);
        }
        else
        {
            ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        }

        if (ImGui::IsItemClicked() && bShowChildren == bWasShowingChildren)
        {
            engine->entities.assign_or_replace<Inspecting>(entity);
        }

        if (const char *templateName = getUsedTemplateName(entity))
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", templateName);
        }

        if (bIsParent && bShowChildren)
        {
            for (auto &[childName, childEntity] : parentComponent->childrenByName)
            {
                std::string displayChildName = "";
                if (const char *globalChildName = engine->getName(childEntity))
                {
                    displayChildName = globalChildName;
                    displayChildName += " ";
                }
                displayChildName += "[child '" + childName + "']";
                drawEntity(childEntity, displayChildName);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };

    for (auto &[name, entity] : engine->getNamedEntities())
    {
        bool bPassedFilters = true;
        if (templateFilter.IsActive())
        {
            if (const char *templateName = getUsedTemplateName(entity))
            {
                bPassedFilters &= templateFilter.PassFilter(templateName);
            }
            else
            {
                for (const ImGuiTextFilter::ImGuiTextRange &filter : templateFilter.Filters)
                {
                    // If we have an including-filter, exclude all without a template.
                    // But if we only have excluding-filters, include all without a template.
                    if (*filter.b != '-')
                    {
                        bPassedFilters = false;
                        break;
                    }
                }
            }
        }
        bPassedFilters &= nameFilter.PassFilter(name.c_str());
        if (bPassedFilters)
        {
            drawEntity(entity, name);
        }
    }
}

void dibidab::ecs::Inspector::drawInspectById()
{
    bool bHovered = false;

    static int id = 0;
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Inspect by ID", &id);
    bHovered |= ImGui::IsItemHovered();

    ImGui::SameLine();

    const entt::entity entity = entt::entity(id);
    const bool bValid = engine->entities.valid(entity);

    if (ImGui::Button("Inspect") && bValid)
    {
        engine->entities.get_or_assign<Inspecting>(entity);
    }
    bHovered |= ImGui::IsItemHovered();

    if (bHovered)
    {
        if (bValid)
        {
            std::string entityDescription = "#" + std::to_string(id);
            if (const char *name = engine->getName(entity))
            {
                entityDescription += " '";
                entityDescription += name;
                entityDescription += "'";
            }
            if (const char *templateName = getUsedTemplateName(entity))
            {
                entityDescription += " (";
                entityDescription += templateName;
                entityDescription += ")";
            }
            ImGui::SetTooltip("%s", entityDescription.c_str());
        }
        else
        {
            ImGui::SetTooltip("#%d does not exist!", id);
        }
    }
}

void dibidab::ecs::Inspector::drawCreateEntityFromTemplate()
{
    LuaTemplate *luaTemplate = dynamic_cast<LuaTemplate *>(creatingEntity.fromTemplate);
    if (luaTemplate == nullptr || luaTemplate->getDefaultArgs().empty())
    {
        creatingEntity.fromTemplate->create(creatingEntity.bPersistent);
        creatingEntity = CreatingEntity();
        return;
    }

    static TemplateArgs args;

    static LuaTemplate *prevLuaTemplate = nullptr;
    if (prevLuaTemplate != luaTemplate)
    {
        prevLuaTemplate = luaTemplate;
        args.createFunctionArguments = luaTemplate->getDefaultArgs();
    }

    ImGui::SetNextWindowPos(vec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(vec2(400, 300), ImGuiCond_Once);

    const std::string title = "Creating " + luaTemplate->name;
    bool bOpen = true;
    if (ImGui::Begin(title.c_str(), &bOpen, ImGuiWindowFlags_NoSavedSettings))
    {
        // TODO: Ideally there is a Json editor, so we do not need the wrapper struct..
        static StructInspector editor(*dibidab::findStructInfo(typename_utils::getTypeName<TemplateArgs>(false).c_str()));
        json jsonArgs = args;
        if (editor.draw(jsonArgs))
        {
            args = jsonArgs;
        }
        ImGui::Separator();
        ImGui::Checkbox("Persistent", &creatingEntity.bPersistent);
        ImGui::Checkbox("Inspect on Create", &creatingEntity.bInspectOnCreate);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save the created entity to level file");
        }
        if (ImGui::Button("Create"))
        {
            bOpen = false;

            const entt::entity createdEntity = engine->entities.create();
            luaTemplate->createComponentsWithJsonArguments(createdEntity, args.createFunctionArguments, creatingEntity.bPersistent);
            if (creatingEntity.bInspectOnCreate)
            {
                engine->entities.assign_or_replace<Inspecting>(createdEntity);
            }
        }
    }
    ImGui::End();

    if (!bOpen)
    {
        creatingEntity = CreatingEntity();
    }
}

void dibidab::ecs::Inspector::drawBehaviorTreeInspectors()
{
    engine->entities.view<InspectingBrain>(entt::exclude<behavior::TreeInspector>).each([&] (entt::entity entity, auto)
    {
        engine->entities.assign<behavior::TreeInspector>(entity, *engine, entity);
    });

    engine->entities.view<behavior::TreeInspector>().each([&] (entt::entity entity, behavior::TreeInspector &inspector)
    {
        engine->entities.get_or_assign<InspectingBrain>(entity);
        if (!inspector.drawGUI())
        {
            engine->entities.remove<behavior::TreeInspector>(entity);
            engine->entities.remove<InspectingBrain>(entity);
        }
    });
}

dibidab::ecs::Template *dibidab::ecs::Inspector::getUsedTemplate(entt::entity entity) const
{
    if (const LuaScripted *scripted = engine->entities.try_get<LuaScripted>(entity))
    {
        if (scripted->usedTemplate != nullptr)
        {
            return scripted->usedTemplate;
        }
    }
    return nullptr;
}

std::set<const dibidab::ComponentInfo *> dibidab::ecs::Inspector::getComponentsForEntity(entt::entity entity) const
{
    std::set<const dibidab::ComponentInfo *> infos;
    for (const auto &[name, info] : dibidab::getAllComponentInfos())
    {
        if (info.hasComponent(entity, engine->entities))
        {
            infos.insert(&info);
        }
    }
    return infos;
}

void dibidab::ecs::Inspector::addCustomDrawFunctions(StructInspector &structEditor)
{
    structEditor.customFieldDraw[typename_utils::getTypeName<entt::entity>(false)] = [&] (json &entityJson)
    {
        bool bEdited = false;
        int32 entityInt = entityJson.is_null() ? int32(entt::entity(entt::null)) : int32(entityJson);
        ImGui::SetNextItemWidth(64);
        if (ImGui::InputScalar("", ImGuiDataType_S32, &entityInt))
        {
            entityJson = entityInt;
            bEdited = true;
        }
        bool bHovered = ImGui::IsItemHovered();
        ImGui::SameLine();
        const entt::entity entity = entt::entity(entityInt);
        if (engine->entities.valid(entity))
        {
            if (ImGui::Button("Inspect"))
            {
                engine->entities.get_or_assign<Inspecting>(entity);
            }
            bHovered |= ImGui::IsItemHovered();
        }
        else if (entity == entt::null)
        {
            ImGui::ButtonEx("Null", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
        }
        else
        {
            ImGui::ButtonEx("Invalid!", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
        }
        if (bHovered && engine->entities.valid(entity))
        {
            if (const char *name = engine->getName(entity))
            {
                ImGui::SetTooltip("%s", name);
            }
        }
        return bEdited;
    };
    structEditor.customFieldDraw[typename_utils::getTypeName<PersistentRef>(false)] = [&] (json &refJson)
    {
        PersistentRef ref = refJson;
        entt::entity resolvedEntity = entt::null;
        const bool bResolved = ref.tryResolve(engine->entities, resolvedEntity);

        if (!bResolved)
        {
            ImGui::ButtonEx("Unresolved!", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
        }
        else if (resolvedEntity == entt::null)
        {
            ImGui::ButtonEx("Null", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
        }
        else if (!engine->entities.valid(resolvedEntity))
        {
            ImGui::ButtonEx("Invalid!", ImVec2(0, 0), ImGuiButtonFlags_Disabled);
        }
        else
        {
            const std::string btnTxt = "#" + std::to_string(int(resolvedEntity));
            if (ImGui::Button(btnTxt.c_str()))
            {
                engine->entities.assign<Inspecting>(resolvedEntity);
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Persistent Id: %lu", ref.getId());
        }
        return false;
    };
}
