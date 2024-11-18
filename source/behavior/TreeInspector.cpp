#include "TreeInspector.h"

#include "nodes/ComponentObserverNode.h"

#include <utils/string_utils.h>

#include <imgui.h>

bool dibidab::behavior::TreeInspector::drawGUI(Tree &tree, const std::string &title)
{
    bool bOpen = true;

    if (ImGui::Begin("Behavior Tree Inspector"))
    {
        // TODO: something is broken here (if more than 1 tab is opened):
        if (ImGui::BeginTabBar("Trees"))
        {
            if (ImGui::BeginTabItem(title.c_str(), &bOpen))
            {
                if (dibidab::behavior::Tree::Node *root = tree.getRootNode())
                {
                    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.back()); // small default font
                    ImGui::Columns(3);
                    ImGui::SetColumnWidth(1, 96.0f);
                    drawNode(root, 0u);
                    ImGui::Columns(1);
                    ImGui::PopFont();
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

void dibidab::behavior::TreeInspector::drawNode(Tree::Node *node, uint depth)
{
    ImGui::PushID(node);
    ImGui::AlignTextToFramePadding();

    ImGuiTreeNodeFlags nodeFlags = 0;
    if (Tree::LeafNode *leafNode = dynamic_cast<Tree::LeafNode *>(node))
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
        if (ComponentObserverNode *observerParent = dynamic_cast<ComponentObserverNode *>(node->parent))
        {
            bool bNodeIsFulfilledBranch = false;
            if (observerParent->fulfilledNodeIndex >= 0)
            {
                bNodeIsFulfilledBranch = observerParent->getChildren()[observerParent->fulfilledNodeIndex] == node;
            }
            nodeName = (bNodeIsFulfilledBranch ? "[Fulfilled] " : "[Unfulfilled] ") + nodeName;
        }
    }

    bool bNodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), nodeFlags);

    if (node->source.path != nullptr)
    {
        std::vector<std::string> pathSplitted = su::split(node->source.path, "/");
        if (!pathSplitted.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s:%d", pathSplitted.back().c_str(), node->source.line);
        }
    }
    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();

    if (node->isEntered())
    {
        if (node->isAborting())
        {
            ImGui::SameLine();
            ImGui::Text("[ABORTING...]");
        }
    }
    else if (node->hasFinishedAtLeastOnce())
    {
        switch (node->getLastResult())
        {
            case Tree::Node::Result::SUCCESS:
                ImGui::TextDisabled("[SUCCESS]");
                break;
            case Tree::Node::Result::FAILURE:
                ImGui::TextDisabled("[FAILURE]");
                break;
            case Tree::Node::Result::ABORTED:
                ImGui::TextDisabled("[ABORTED]");
                break;
        }
    }

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    node->drawDebugInfo();

    ImGui::NextColumn();

    if (bNodeOpen)
    {
        if (Tree::CompositeNode *composite = dynamic_cast<Tree::CompositeNode *>(node))
        {
            for (Tree::Node *child : composite->getChildren())
            {
                drawNode(child, depth + 1u);
            }
        }
        else if (Tree::DecoratorNode *decorator = dynamic_cast<Tree::DecoratorNode *>(node))
        {
            if (Tree::Node *child = decorator->getChild())
            {
                drawNode(child, depth + 1u);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
