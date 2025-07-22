#include "NPCFactory.h"

NPCFactory& NPCFactory::getSingleton() {
    static NPCFactory instance;
    return instance;
}

void NPCFactory::registerType(const std::string& name, Creator c) {
    mCreators[name] = c;
}

npc_template* NPCFactory::create(const std::string& name) {
    auto it = mCreators.find(name);
    if (it != mCreators.end()) {
        return it->second();
    }
    return nullptr;
}
