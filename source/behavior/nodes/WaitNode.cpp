#include "WaitNode.h"

#include "../../level/Level.h"
#include "../../ecs/systems/TimeOutSystem.h"

#include <imgui.h>

dibidab::behavior::WaitNode::WaitNode() :
    seconds(-1.0f),
    waitingEntity(entt::null),
    engine(nullptr)
#ifndef NDEBUG
    ,
    timeStarted(0.0f)
#endif
{}

dibidab::behavior::WaitNode *dibidab::behavior::WaitNode::finishAfter(const float inSeconds, const entt::entity inWaitingEntity,
    ecs::Engine *inEngine)
{
    seconds = inSeconds;
    waitingEntity = inWaitingEntity;
    engine = inEngine;
#ifndef NDEBUG
    if (inEngine == nullptr)
    {
        throw gu_err("Engine is a nullptr!\n" + getReadableDebugInfo());
    }
    if (!inEngine->entities.valid(inWaitingEntity))
    {
        throw gu_err("Entity is not valid!\n" + getReadableDebugInfo());
    }
#endif
    return this;
}

void dibidab::behavior::WaitNode::enter()
{
    Node::enter();
    if (seconds >= 0.0f && engine != nullptr)
    {
        if (!engine->entities.valid(waitingEntity))
        {
            throw gu_err("Entity #" + std::to_string(int(waitingEntity)) + " is not valid!\n" + getReadableDebugInfo());
        }
        onWaitFinished = engine->getTimeOuts()->unsafeCallAfter(seconds, waitingEntity, [&]()
        {
            finish(Node::Result::SUCCESS);
        });
#ifndef NDEBUG
        if (level::Room *room = dynamic_cast<level::Room *>(engine))
        {
            timeStarted = float(room->getLevel().getTime());
        }
#endif
    }
}

void dibidab::behavior::WaitNode::abort()
{
    Node::abort();
    finish(Node::Result::ABORTED);
}

void dibidab::behavior::WaitNode::finish(Node::Result result)
{
    onWaitFinished.reset();
    Node::finish(result);
}

const char *dibidab::behavior::WaitNode::getName() const
{
    return "Wait";
}

void dibidab::behavior::WaitNode::drawDebugInfo() const
{
    Node::drawDebugInfo();
    const bool bUsingTime = seconds >= 0.0f && engine != nullptr;
    if (bUsingTime)
    {
        ImGui::Text("%.2fs", seconds);
    }
    else
    {
        ImGui::TextDisabled("Until abort");
    }
    if (ImGui::IsItemHovered())
    {
        if (engine != nullptr && engine->entities.valid(waitingEntity))
        {
            if (const char *waitingEntityName = engine->getName(waitingEntity))
            {
                ImGui::SetTooltip("Entity: #%d %s", int(waitingEntity), waitingEntityName);
            }
            else
            {
                ImGui::SetTooltip("Entity: #%d", int(waitingEntity));
            }
        }
    }
#ifndef NDEBUG
    if (bUsingTime && isEntered())
    {
        if (level::Room *room = dynamic_cast<level::Room *>(engine))
        {
            ImGui::SameLine();
            const float timeElapsed = float(room->getLevel().getTime()) - timeStarted;
            ImGui::ProgressBar(timeElapsed / seconds, ImVec2(100.0f, 0.0f), std::string(std::to_string(int(timeElapsed))
                + "." + std::to_string(int(fract(timeElapsed) * 10.0f)) + "s").c_str());
        }
    }
#endif

}
