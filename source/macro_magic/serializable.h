#pragma once
#include "SerializableStructInfo.h"
#include "dibidab/converters/lua_converters.h"
#include "dibidab/converters/json_converters.h"
#include <json.hpp>


template <class FieldType>
bool isFieldTypePrimitive()
{
    return false;
}

/**
 * assume that a FieldType is fixed size if a new instance already contains items.
 *
 * if there are no items, then assume that FieldType will accept new items.     // kinda TODO. this is EXTREMELY hacky, but so far it works perfect
 */
template <class FieldType>
bool isStructFieldFixedSize()
{
    return false;
}
