#include "EntityInspector.h"

#include "EntityEngine.h"

#include "systems/EntitySystem.h"
#include "components/Inspecting.dibidab.h"
#include "components/LuaScripted.dibidab.h"
#include "components/Children.dibidab.h"
#include "components/Brain.dibidab.h"

#include "../ai/behavior_trees/BehaviorTreeInspector.h"
#include "../game/dibidab.h"

#include <assets/AssetManager.h>
#include <input/mouse_input.h>
#include <utils/string_utils.h>
#include <code_editor/CodeEditor.h>
#include <files/file_utils.h>
#include <gu/profiler.h>

#include <imgui_internal.h>

EntityMover::EntityMover(entt::entity entity) :
    entity(entity)
{
}

EntityInspector::EntityInspector(EntityEngine &engine) :
    engine(&engine)
{
}

void EntityInspector::draw()
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
}

EntityInspector::~EntityInspector()
{
    delete picker;
    delete mover;
}

EntityPicker *EntityInspector::createPicker()
{
    return nullptr;
}

EntityMover *EntityInspector::createMover(const entt::entity entityToMove)
{
    return nullptr;
}

const char *EntityInspector::getUsedTemplateName(entt::entity entity) const
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

const loaded_asset *EntityInspector::getAssetForTemplate(const EntityTemplate &entityTemplate) const
{
    if (const LuaEntityTemplate *luaEntityTemplate = dynamic_cast<const LuaEntityTemplate *>(&entityTemplate))
    {
        return luaEntityTemplate->script.getLoadedAsset();
    }
    return nullptr;
}

void EntityInspector::editTemplateAsset(const loaded_asset &templateAsset)
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

void EntityInspector::updatePicking()
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

void EntityInspector::updateMoving()
{
    if (!mover->update())
    {
        delete mover;
        mover = nullptr;
    }
}

void EntityInspector::drawInspectWindow(const entt::entity entity, Inspecting &inspecting, bool &bKeepInspecting, bool &bDestroyEntity)
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

    if (EntityTemplate *usedTemplate = getUsedTemplate(entity))
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
        if (ImGui::Button("Inspect brain") && !engine->entities.has<BehaviorTreeInspector>(entity))
        {
            engine->entities.assign<BehaviorTreeInspector>(entity, *engine, entity);
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

void EntityInspector::drawEntityNameField(const entt::entity entity)
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

void EntityInspector::drawMainMenuItem()
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

        if (ImGui::BeginMenu("Entity Tree"))
        {
            drawEntityRegistryTree();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Enabled Systems"))
        {
            for (const EntitySystem *system : engine->getSystems())
            {
                ImGui::MenuItem(system->name.c_str(), nullptr, system->bUpdatesEnabled);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void EntityInspector::drawEntityTemplateSelect()
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
        EntityTemplate &entityTemplate = engine->getTemplate(templateName);
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


void EntityInspector::drawEntityRegistryTree()
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

void EntityInspector::drawInspectById()
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

void EntityInspector::drawCreateEntityFromTemplate()
{
    LuaEntityTemplate *luaTemplate = dynamic_cast<LuaEntityTemplate *>(creatingEntity.fromTemplate);
    if (luaTemplate == nullptr || luaTemplate->getDefaultArgs().empty())
    {
        creatingEntity.fromTemplate->create(creatingEntity.bPersistent);
        creatingEntity = CreatingEntity();
        return;
    }

    static json args;

    static LuaEntityTemplate *prevLuaTemplate = nullptr;
    if (prevLuaTemplate != luaTemplate)
    {
        prevLuaTemplate = luaTemplate;
        args = luaTemplate->getDefaultArgs();
    }

    ImGui::SetNextWindowPos(vec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(vec2(400, 300), ImGuiCond_Once);

    const std::string title = "Creating " + luaTemplate->name;
    bool bOpen = true;
    if (ImGui::Begin(title.c_str(), &bOpen, ImGuiWindowFlags_NoSavedSettings))
    {
        // TODO: Json tree.
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
            luaTemplate->createComponentsWithJsonArguments(createdEntity, args, creatingEntity.bPersistent);
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

void EntityInspector::drawBehaviorTreeInspectors()
{
    engine->entities.view<InspectingBrain>(entt::exclude<BehaviorTreeInspector>).each([&] (entt::entity entity, auto)
    {
        engine->entities.assign<BehaviorTreeInspector>(entity, *engine, entity);
    });

    engine->entities.view<BehaviorTreeInspector>().each([&] (entt::entity entity, BehaviorTreeInspector &inspector)
    {
        engine->entities.get_or_assign<InspectingBrain>(entity);
        if (!inspector.drawGUI())
        {
            engine->entities.remove<BehaviorTreeInspector>(entity);
            engine->entities.remove<InspectingBrain>(entity);
        }
    });
}

EntityTemplate *EntityInspector::getUsedTemplate(entt::entity entity) const
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
