
#include "BehaviorTreeInspector.h"

#include "../../generated/Brain.hpp"

#include <imgui.h>

BehaviorTreeInspector::BehaviorTreeInspector(EntityEngine &engine, entt::entity entity) :
    engine(&engine),
    entity(entity)
{

}

bool BehaviorTreeInspector::drawGUI()
{
    ImGui::ShowDemoWindow();
    if (!engine->entities.valid(entity) || !engine->entities.has<Brain>(entity))
    {
        return false;
    }
    Brain &brain = engine->entities.get<Brain>(entity);
    BehaviorTree &tree = brain.behaviorTree;

    bool bOpen = true;

    if (ImGui::Begin("Behavior Tree Inspector"))
    {
        if (ImGui::BeginTabBar("Entities"))
        {
            std::string tabName = "#" + std::to_string(int(entity));
            if (const char *entityName = engine->getName(entity))
            {
                tabName += " ";
                tabName += entityName;
            }
            if (ImGui::BeginTabItem(tabName.c_str(), &bOpen))
            {
                if (BehaviorTree::Node *root = tree.getRootNode())
                {
                    ImGui::Columns(3);
                    ImGui::SetColumnWidth(1, 96.0f);
                    drawNode(root, 0u);
                    ImGui::Columns(1);
                }
                else
                {
                    ImGui::Text("No root node!");
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

    }
    ImGui::End();
    return bOpen;
}

void BehaviorTreeInspector::drawNode(BehaviorTree::Node *node, uint depth)
{
    ImGui::PushID(node);
    ImGui::AlignTextToFramePadding();

    ImGuiTreeNodeFlags nodeFlags = 0;
    if (BehaviorTree::LeafNode *leafNode = dynamic_cast<BehaviorTree::LeafNode *>(node))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (depth < 4)
    {
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (node->isEntered())
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string nodeName = node->getName();
    if (node->parent)
    {
        if (BehaviorTree::ComponentObserverNode *observerParent = dynamic_cast<BehaviorTree::ComponentObserverNode *>(node->parent))
        {
            bool bNodeIsFulfilledBranch = false;
            if (observerParent->fulfilledNodeIndex > 0)
            {
                bNodeIsFulfilledBranch = observerParent->getChildren()[observerParent->fulfilledNodeIndex] == node;
            }
            nodeName = (bNodeIsFulfilledBranch ? "[Fulfilled] " : "[Unfulfilled] ") + nodeName;
        }
    }

    bool bNodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), nodeFlags);

    if (node->hasLuaDebugInfo())
    {
        const lua_Debug &debugInfo = node->getLuaDebugInfo();
        std::vector<std::string> pathSplitted = splitString(debugInfo.source, "/");
        if (!pathSplitted.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s:%d", pathSplitted.back().c_str(), debugInfo.currentline);
        }
    }
    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();

    if (node->isEntered())
    {
        if (node->isAborted())
        {
            ImGui::SameLine();
            ImGui::Text("[ABORTING...]");
        }
    }
#ifndef NDEBUG
    else if (node->hasFinishedAtLeastOnce())
    {
        switch (node->getLastResult())
        {
            case BehaviorTree::Node::Result::SUCCESS:
                ImGui::TextDisabled("[SUCCESS]");
                break;
            case BehaviorTree::Node::Result::FAILURE:
                ImGui::TextDisabled("[FAILURE]");
                break;
            case BehaviorTree::Node::Result::ABORTED:
                ImGui::TextDisabled("[ABORTED]");
                break;
        }
    }
#endif

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    node->drawDebugInfo();

    ImGui::NextColumn();

    if (bNodeOpen)
    {
        if (BehaviorTree::CompositeNode *composite = dynamic_cast<BehaviorTree::CompositeNode *>(node))
        {
            for (BehaviorTree::Node *child : composite->getChildren())
            {
                drawNode(child, depth + 1u);
            }
        }
        else if (BehaviorTree::DecoratorNode *decorator = dynamic_cast<BehaviorTree::DecoratorNode *>(node))
        {
            if (BehaviorTree::Node *child = decorator->getChild())
            {
                drawNode(child, depth + 1u);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
