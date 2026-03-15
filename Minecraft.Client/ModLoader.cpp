#include "ModLoader.h"
#include <filesystem>
#include <cstdlib>

#include "Level/MultiPlayerLevel.h"
#include "Minecraft.h"
#include "../Minecraft.World/Level/Storage/EntityIO.h"

// {-------------------------------------------}
// | SEE MODDING.md for in depth documentation |
// {-------------------------------------------}

ModManager::ModManager(Minecraft* mc) : minecraft(mc) {
    lua.open_libraries(sol::lib::base, 
                      sol::lib::package, 
                      sol::lib::table, 
                      sol::lib::math,
                      sol::lib::string
    );

    lua.set_function("log", [](std::string msg) {
        puts(msg.c_str());
    });

    lua.set_function("spawn_entity", [this](int entityId, float x, float y, float z) {
        if (!this->minecraft || !this->minecraft->level) return;

        std::shared_ptr<Entity> entity = EntityIO::newById(entityId, this->minecraft->level);

        if (entity != nullptr) {
            entity->setPos(x, y, z);
            this->minecraft->level->addEntity(entity); 
        }
    });
}

void ModManager::loadMods(const std::string& directory) {
    if (!std::filesystem::exists(directory)) return;

    std::lock_guard<std::mutex> lock(luaLock);
    
    tickCallbacks.clear();
    modEnvironments.clear();
    loadedModNames.clear();

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".lua") {
            std::wstring wModName = entry.path().filename().wstring();
            std::string path = entry.path().string();
            
            if (modEnabledMap.find(wModName) == modEnabledMap.end()) {
                modEnabledMap[wModName] = false;
            }

            sol::environment modEnv(lua, sol::create, lua.globals());
            auto loadResult = lua.script_file(path, modEnv, sol::script_pass_on_error);
            
            if (!loadResult.valid()) {
                sol::error err = loadResult;
                printf("[Lua] [ERR] %s: %s\n", path.c_str(), err.what());
                continue;
            }

            sol::optional<sol::function> onTick = modEnv["on_tick"];
            if (onTick && onTick->is<sol::function>()) {
                tickCallbacks.push_back(*onTick);
            }

            modEnvironments.push_back(modEnv);
            loadedModNames.push_back(wModName);
        }
    }
}

void ModManager::update() {
    std::lock_guard<std::mutex> lock(luaLock);
    
    for (size_t i = 0; i < modEnvironments.size(); ++i) {
        std::wstring& name = loadedModNames[i];
        
        if (modEnabledMap.count(name) && !modEnabledMap[name]) {
            continue;
        }

        sol::optional<sol::function> tick = modEnvironments[i]["on_tick"];
        if (tick && tick->is<sol::function>()) {
            auto result = (*tick)();
            if (!result.valid()) {
                sol::error err = result;
            }
        }
    }
}