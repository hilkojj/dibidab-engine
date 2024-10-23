#pragma once

#include <json_fwd.hpp>

#include <map>
#include <string>
#include <functional>

namespace dibidab
{
    struct StructInfo;
    struct VariableInfo;
}

class StructEditor
{
  public:
    StructEditor(const dibidab::StructInfo &structInfo);

    bool draw(json &structJson);

    // Functions for drawing fields for custom types. Function should return true on edit.
    std::map<std::string /* typename */, std::function<bool(json &)>> customFieldDraw;

    // Error that is presented to user when parsing Json fails.
    std::string jsonParseError;

  private:
    bool drawKeyValue(json &key, json &value, const std::string &keyType, const std::string &valueType, bool bEditKey, bool *bRemove = nullptr);

    bool drawField(json &value, const std::string &valueType);

    bool drawStringField(std::string &string, const char *previewText = nullptr);

    bool drawStructure(json &structure, const std::string &structureType);

    bool drawInsertIntoStructure(json &structure, const std::string &keyType, const std::string &valueType, bool bInsertAsPair = false);

    bool drawSubStructEditor(const dibidab::StructInfo &subStructInfo, json &subStructJson);

    bool inferArrayValueType(const std::string &fullArrayType, std::string &outValueType) const;

    bool isMapType(const std::string &fullMapType, std::string *outMapType = nullptr) const ;

    bool inferMapKeyValueTypes(const std::string &fullMapType, std::string &outKeyType, std::string &outValueType) const;

    bool isVecType(const std::string &type, int &outSize, int &outImGuiDataType) const;

    const dibidab::StructInfo *structInfo;

    // Struct Editors for values stored inside our struct. Key is latest ImGui ID from the window stack.
    std::map<unsigned int, StructEditor> subStructEditors;

    // String buffers for input. Key is latest ImGui ID from the window stack.
    std::map<unsigned int, std::string> stringInput;
};
