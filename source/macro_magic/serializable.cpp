#include "serializable.h"

std::map<std::string, SerializableStructInfo *> *SerializableStructInfo::infos = nullptr;
std::map<std::size_t , SerializableStructInfo *> *SerializableStructInfo::infosByType = nullptr;
