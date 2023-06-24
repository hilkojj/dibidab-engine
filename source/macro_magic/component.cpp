//
// Created by kaasflip on 08/04/2020.
//
#include "component.h"

std::map<std::size_t, ComponentUtils *> *ComponentUtils::utilsByType = nullptr;
std::map<std::string, ComponentUtils *> *ComponentUtils::utils = nullptr;
std::vector<std::string> *ComponentUtils::names = nullptr;
