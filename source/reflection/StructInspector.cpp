
#include "StructInspector.h"

#include "StructInfo.h"

#include "utils/type_name.h"
#include "utils/string_utils.h"

#include "json.hpp"
#include "imgui.h"
#include "imgui_internal.h"

#include <list>

dibidab::StructInspector::StructInspector(const dibidab::StructInfo &structInfo) :
    structInfo(&structInfo)
{

}

bool dibidab::StructInspector::draw(json &structJson)
{
    const int prevColumns = ImGui::GetColumnsCount();
    if (prevColumns != 2)
    {
        ImGui::Columns(2);
    }

    bool bEdited = false;

    int varIndex = 0;
    for (const dibidab::VariableInfo &var : structInfo->variables)
    {
        if (!var.bJsonExposed)
        {
            continue;
        }
        json key = var.name;
        json &value = structJson.is_object() ? structJson[var.name] : structJson[varIndex];
        bEdited |= drawKeyValue(key, value, "", var.typeName, false);
        ++varIndex;
    }

    if (prevColumns != 2)
    {
        ImGui::Columns(prevColumns);
    }

    ImGui::PushID(this);
    if (!jsonParseError.empty() && !ImGui::IsPopupOpen("JsonParseError"))
    {
        ImGui::OpenPopup("JsonParseError");
    }
    if (ImGui::BeginPopup("JsonParseError"))
    {
        ImGui::Text("%s", jsonParseError.c_str());
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || jsonParseError.empty())
        {
            jsonParseError.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return bEdited;
}

bool dibidab::StructInspector::drawKeyValue(json &key, json &value, const std::string &keyType, const std::string &valueType, const bool bEditKey, bool *bRemove)
{
    ImGui::PushID(key.dump().c_str());
    ImGui::AlignTextToFramePadding();

    bool bEdited = false;
    bool bOpened = false;

    if (bRemove != nullptr)
    {
        if (ImGui::CloseButton(ImGui::GetID("Remove"), ImGui::GetCursorScreenPos()))
        {
            *bRemove = true;
        }
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetItemRectSize().x);
    }

    if (bEditKey)
    {
        // Show field for editing key:
        ImGui::SetNextItemWidth(72);
        ImGui::PushID("Key");
        bEdited |= drawField(key, keyType);
        ImGui::PopID();
        ImGui::SameLine();
    }

    std::string label = "";
    if (!bEditKey)
    {
        label = key.is_string() ? std::string(key) : key.dump();
    }

    label += "##" + key.dump();
    if (value.is_structured())
    {
        bOpened = ImGui::TreeNode(label.c_str());
    }
    else
    {
        ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    }

    // Do not show type of value next to key when key is editable, because that is confusing.
    if (!bEditKey)
    {
        // Show type of value:
        ImGui::SameLine();
        ImGui::TextDisabled("%s", valueType.c_str());
    }

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();

    bEdited |= drawField(value, valueType);

    ImGui::NextColumn();

    if (bOpened)
    {
        bEdited |= drawStructure(value, valueType);
        ImGui::Separator();
        ImGui::TreePop();
    }
    ImGui::PopID();
    return bEdited;
}

bool dibidab::StructInspector::drawField(json &value, const std::string &valueType)
{
    bool bEdited = false;

    int vecSize = 0;
    ImGuiDataType vecDataType = 0;

    auto customDrawIt = customFieldDraw.find(valueType);
    if (customDrawIt != customFieldDraw.end())
    {
        bEdited = customDrawIt->second(value);
    }
    else if (isVecType(valueType, vecSize, vecDataType) && value.is_array() && value.size() == vecSize)
    {
        const float widthPerInput = ImGui::CalcItemWidth() / vecSize;
        json editedVec = value;
        for (int i = 0; i < vecSize; i++)
        {
            ImGui::PushID(i);
            if (i > 0)
            {
                ImGui::SameLine();
            }
            ImGui::SetNextItemWidth(widthPerInput);
            if (vecDataType == ImGuiDataType_Float)
            {
                float floatValue = value[i];
                if (ImGui::InputScalar("", vecDataType, &floatValue))
                {
                    editedVec[i] = floatValue;
                }
            }
            else if (vecDataType == ImGuiDataType_U8 || vecDataType == ImGuiDataType_U16 || vecDataType == ImGuiDataType_U32 || vecDataType == ImGuiDataType_U64)
            {
                uint64 uintValue = value[i];
                if (ImGui::InputScalar("", ImGuiDataType_U64, &uintValue))
                {
                    editedVec[i] = uintValue;
                }
            }
            else
            {
                int64 intValue = value[i];
                if (ImGui::InputScalar("", ImGuiDataType_S64, &intValue))
                {
                    editedVec[i] = intValue;
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                bEdited = true;
                value = editedVec;
            }
            ImGui::PopID();
        }
    }
    else if (value.is_boolean())
    {
        bool bValue = value;
        if (ImGui::Checkbox("", &bValue))
        {
            value = bValue;
            bEdited = true;
        }
    }
    else if (value.is_number_float())
    {
        float floatValue = value;
        if ((ImGui::DragFloat("##FloatDrag", &floatValue) && !ImGui::TempInputIsActive(ImGui::GetID("##FloatDrag"))) || ImGui::IsItemDeactivatedAfterEdit())
        {
            value = floatValue;
            bEdited = true;
        }
    }
    else if (value.is_number_unsigned())
    {
        uint64 uintValue = value;
        if (ImGui::InputScalar("", ImGuiDataType_U64, &uintValue) && ImGui::IsItemDeactivatedAfterEdit())
        {
            value = uintValue;
            bEdited = true;
        }
    }
    else if (value.is_number_integer())
    {
        int64 intValue = value;
        if (ImGui::InputScalar("", ImGuiDataType_S64, &intValue) && ImGui::IsItemDeactivatedAfterEdit())
        {
            value = intValue;
            bEdited = true;
        }
    }
    else if (value.is_string())
    {
        std::string stringValue = value;
        if (drawStringField(stringValue))
        {
            value = stringValue;
            bEdited = true;
        }
    }
    else
    {
        ImGui::TextDisabled("%s", value.dump().c_str());
    }
    return bEdited;
}

bool dibidab::StructInspector::drawStringField(std::string &string, const char *previewText)
{
    const int extraBuffer = 1024;
    char *ptr = new char[string.length() + extraBuffer]();
    memcpy(ptr, &string[0], string.length());
    if (ImGui::InputTextWithHint("", previewText, ptr, string.length() + extraBuffer))
    {
        string = std::string(ptr);
    }
    delete[] ptr;
    return ImGui::IsItemDeactivatedAfterEdit();
}

bool dibidab::StructInspector::drawStructure(json &structure, const std::string &structureType)
{
    static std::string jsonParseError;
    bool bEdited = false;
    if (const dibidab::StructInfo *subStructInfo = dibidab::findStructInfo(structureType.c_str()))
    {
        // Struct inside struct.
        bEdited |= drawSubStructEditor(*subStructInfo, structure);
    }
    else if (structure.is_object())
    {
        // Map with string-like keys.
        std::string mapKeyType;
        std::string mapValueType;
        inferMapKeyValueTypes(structureType, mapKeyType, mapValueType);

        bool bReplacingMap = false;
        json replacementMap;
        std::vector<std::string> keysToRemove;
        for (auto &[key, value] : structure.items())
        {
            json keyEditable = key;
            bool bRemove = false;
            if (drawKeyValue(keyEditable, value, mapKeyType, mapValueType, true, &bRemove))
            {
                bEdited = true;
                if (bReplacingMap)
                {
                    replacementMap[key] = value;
                }
                if (keyEditable != key)
                {
                    // We can't change the key while iterating. Instead, we make a copy of the map and change the key there.
                    if (!bReplacingMap)
                    {
                        bReplacingMap = true;
                        replacementMap = structure;
                    }
                    replacementMap.erase(replacementMap.find(key));
                    replacementMap[std::string(keyEditable)] = value;
                }
            }
            if (bRemove)
            {
                keysToRemove.push_back(keyEditable);
            }
        }
        if (bReplacingMap)
        {
            structure = replacementMap;
            bEdited = true;
        }
        for (const std::string &key : keysToRemove)
        {
            structure.erase(key);
            bEdited = true;
        }
        bEdited |= drawInsertIntoStructure(structure, mapKeyType, mapValueType, false);
    }
    else if (isMapType(structureType))
    {
        // Map, but stored as array pairs because key is not string-like: [[key, value], [key, value], ...]
        std::string mapKeyType;
        std::string mapValueType;
        inferMapKeyValueTypes(structureType, mapKeyType, mapValueType);

        std::vector<json> keysToRemove;
        for (json &pair : structure)
        {
            if (pair.size() == 2)
            {
                bool bRemove = false;
                json keyEditable = pair[0];
                if (drawKeyValue(keyEditable, pair[1], mapKeyType, mapValueType, true, &bRemove))
                {
                    bEdited = true;
                    if (keyEditable != pair[0])
                    {
                        bool bKeyUnique = true;
                        for (json &otherPair : structure)
                        {
                            if (otherPair.size() == 2 && otherPair[0] == keyEditable)
                            {
                                bKeyUnique = false;
                                break;
                            }
                        }
                        if (bKeyUnique)
                        {
                            pair[0] = keyEditable;
                        }
                    }
                }
                if (bRemove)
                {
                    keysToRemove.push_back(keyEditable);
                }
            }
        }
        for (const json &key : keysToRemove)
        {
            auto it = structure.begin();
            while (it != structure.end())
            {
                if (it->is_array() && (*it)[0] == key)
                {
                    structure.erase(it);
                    bEdited = true;
                    break;
                }
                ++it;
            }
        }
        bEdited |= drawInsertIntoStructure(structure, mapKeyType, mapValueType, true);
    }
    else if (structure.is_array())
    {
        // Array or similar.
        std::string arrayValueType;
        inferArrayValueType(structureType, arrayValueType);
        std::list<int> indicesToRemove;
        int i = 0;
        for (json &value : structure)
        {
            const bool bCanMoveUp = i > 0;
            ImGui::ArrowButtonEx("MoveUp", ImGuiDir_Up, vec2(ImGui::GetFrameHeight()), bCanMoveUp ? ImGuiButtonFlags_None : ImGuiButtonFlags_Disabled);
            if (bCanMoveUp && ImGui::IsItemClicked()) // Note: ImGui::ArrowButtonEx did not return true on click
            {
                value.swap(structure[i - 1]);
                bEdited = true;
            }
            ImGui::SameLine();
            const bool bCanMoveDown = i < structure.size() - 1;
            ImGui::ArrowButtonEx("MoveDown", ImGuiDir_Down, vec2(ImGui::GetFrameHeight()), bCanMoveDown ? ImGuiButtonFlags_None : ImGuiButtonFlags_Disabled);
            if (bCanMoveDown && ImGui::IsItemClicked())
            {
                value.swap(structure[i + 1]);
                bEdited = true;
            }
            ImGui::SameLine();

            bool bRemove = false;
            json key = "[" + std::to_string(i) + "]";
            bEdited |= drawKeyValue(key, value, "", arrayValueType, false, &bRemove);
            if (bRemove)
            {
                indicesToRemove.push_front(i);
            }
            ++i;
        }
        for (int index : indicesToRemove)
        {
            structure.erase(index);
            bEdited = true;
        }
        bEdited |= drawInsertIntoStructure(structure, "", arrayValueType);
    }
    return bEdited;
}

bool dibidab::StructInspector::drawInsertIntoStructure(json &structure, const std::string &keyType, const std::string &valueType, bool bInsertAsPair)
{
    std::string addButtonText;
    bool bShowKeyInput = false;
    if (structure.is_object() || bInsertAsPair)
    {
        addButtonText = "Insert";
        bShowKeyInput = true;
    }
    else
    {
        addButtonText = "Push";
    }
    const float addButtonWidth = ImGui::CalcTextSize(addButtonText.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::AlignTextToFramePadding();

    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()
        + ImGui::GetContentRegionAvailWidth() - addButtonWidth
        - (bShowKeyInput ? ImGui::CalcItemWidth() + ImGui::GetStyle().FramePadding.x : 0.0f)
    );

    ImGui::SetNextItemWidth(addButtonWidth);
    bool bAdd = ImGui::Button(addButtonText.c_str());

    json keyJson;
    if (bShowKeyInput)
    {
        ImGui::SameLine();

        ImGui::PushID("KeyInput");
        std::string &keyString = stringInput[ImGui::GetCurrentWindow()->IDStack.back()];
        if (bInsertAsPair)
        {
            drawStringField(keyString, std::string("JSON for " + keyType).c_str());
            if (bAdd)
            {
                try
                {
                    keyJson = json::parse(keyString);
                }
                catch (std::exception &exception)
                {
                    jsonParseError = exception.what();
                    bAdd = false;
                }
            }
        }
        else
        {
            drawStringField(keyString, keyType.c_str());
            keyJson = keyString;
        }
        ImGui::PopID();
    }

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();

    json valueJson;
    if (const dibidab::StructInfo *valueStructInfo = dibidab::findStructInfo(valueType.c_str()))
    {
        ImGui::TextDisabled("%s", valueStructInfo->id);
        if (bAdd)
        {
            valueJson = valueStructInfo->getDefaultJsonObject();
        }
    }
    else
    {
        ImGui::PushID("JsonParseInput");
        std::string &valueString = stringInput[ImGui::GetCurrentWindow()->IDStack.back()];
        drawStringField(valueString, valueType.empty() ? "JSON literal" : std::string("JSON for " + valueType).c_str());
        ImGui::PopID();
        if (bAdd)
        {
            try
            {
                valueJson = json::parse(valueString);
            }
            catch (const nlohmann::detail::exception &exception)
            {
                jsonParseError = exception.what();
                bAdd = false;
            }
        }
    }

    if (bAdd)
    {
        if (bInsertAsPair)
        {
            structure.push_back({ keyJson, valueJson });
        }
        else if (structure.is_object())
        {
            structure[std::string(keyJson)] = valueJson;
        }
        else if (structure.is_array())
        {
            structure.push_back(valueJson);
        }
    }
    ImGui::NextColumn();
    return bAdd;
}

bool dibidab::StructInspector::drawSubStructEditor(const dibidab::StructInfo &subStructInfo, json &subStructJson)
{
    unsigned int id = ImGui::GetCurrentWindow()->IDStack.back();
    auto subEditorIt = subStructEditors.find(id);
    if (subEditorIt == subStructEditors.end())
    {
        subEditorIt = subStructEditors.insert({ id, StructInspector(subStructInfo) }).first;
    }
    return subEditorIt->second.draw(subStructJson);
}

bool dibidab::StructInspector::inferArrayValueType(const std::string &fullArrayType, std::string &outValueType) const
{
    const static std::vector<std::string> arrayTypes { "std::vector", "std::list", "std::set" };
    for (const std::string &arrayType : arrayTypes)
    {
        if (su::startsWith(fullArrayType, arrayType + "<") && su::endsWith(fullArrayType, ">"))
        {
            outValueType = fullArrayType.substr(arrayType.size() + 1 /* < */, fullArrayType.size() - arrayType.size() - 2 /* < and > */);
            return true;
        }
    }
    return false;
}

bool dibidab::StructInspector::isMapType(const std::string &valueType, std::string *outMapType) const
{
    const static std::vector<std::string> mapTypes { "std::map", "std::unordered_map" };
    for (const std::string &mapType : mapTypes)
    {
        if (su::startsWith(valueType, mapType + "<") && su::endsWith(valueType, ">"))
        {
            if (outMapType != nullptr)
            {
                *outMapType = mapType;
            }
            return true;
        }
    }
    return false;
}

bool dibidab::StructInspector::inferMapKeyValueTypes(const std::string &fullMapType, std::string &outKeyType, std::string &outValueType) const
{
    std::string shortMapType;
    if (isMapType(fullMapType, &shortMapType))
    {
        const std::vector<std::string> templateTypes = su::split(
            fullMapType.substr(shortMapType.size() + 1 /* < */, fullMapType.size() - shortMapType.size() - 2 /* < and > */),
            // ", " is safe to use, the header tool always formats it this way, even if the original header formatted it differently.
            ", "
        );
        if (templateTypes.size() == 2)
        {
            outKeyType = templateTypes[0];
            outValueType = templateTypes[1];
            return true;
        }
    }
    return false;
}

bool dibidab::StructInspector::isVecType(const std::string &type, int &outSize, int &outImGuiDataType) const
{
    static const std::map<std::string, int> prefixToDataType = {
        { "", ImGuiDataType_Float },

        { "i", ImGuiDataType_S32 },
        { "i8", ImGuiDataType_S8 },
        { "i16", ImGuiDataType_S16 },
        { "i32", ImGuiDataType_S32 },
        { "i64", ImGuiDataType_S64 },

        { "u", ImGuiDataType_U32 },
        { "u8", ImGuiDataType_U8 },
        { "u16", ImGuiDataType_U16 },
        { "u32", ImGuiDataType_U32 },
        { "u64", ImGuiDataType_U64 },
    };
    for (const auto &[prefix, dataType] : prefixToDataType)
    {
        for (int size : { 2, 3, 4 })
        {
            if (type == prefix + "vec" + std::to_string(size))
            {
                outSize = size;
                outImGuiDataType = dataType;
                return true;
            }
        }
    }
    return false;
}
