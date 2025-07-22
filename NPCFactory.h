#pragma once
#include <Ogre.h>
#include <map>
#include <string>
#include "npc_template.h"

class NPCFactory {
public:
    typedef npc_template* (*Creator)();
    static NPCFactory& getSingleton();

    void registerType(const std::string& name, Creator c);
    npc_template* create(const std::string& name);
private:
    std::map<std::string, Creator> mCreators;
};
