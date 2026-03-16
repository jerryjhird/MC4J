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

    lua.set_function("print", [](std::string msg) {
        ::write(1, msg.c_str(), msg.length()); 
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
    
    modEnvironments.clear();
    loadedModNames.clear();
    modEnabledMap.clear();

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".lua") {
            std::string path = entry.path().string();
            sol::environment modEnv(lua, sol::create, lua.globals());
            
            auto loadResult = lua.script_file(path, modEnv, sol::script_pass_on_error);
            if (!loadResult.valid()) { // if lua is unable to load file (e.g syntax error)
                sol::error err = loadResult;
                printf("[Lua] [ERR] %s: %s\n", path.c_str(), err.what());
                continue;
            }

            sol::optional<std::string> luaModName = modEnv["ModName"];
            sol::optional<std::string> author = modEnv["ModAuthor"];

            if (!luaModName || luaModName->empty() || !author) {
                printf("[Lua] [REJECTED] %s: Missing ModName or ModAuthor.\n", path.c_str());
                continue; 
            }

            std::string finalName = *luaModName;
            int duplicateCount = 1;

            auto nameExists = [&](const std::string& nameToCheck) {
                std::wstring wCheck(nameToCheck.begin(), nameToCheck.end());
                return std::find(loadedModNames.begin(), loadedModNames.end(), wCheck) != loadedModNames.end();
            };

            while (nameExists(finalName)) {
                finalName = *luaModName + " (" + std::to_string(duplicateCount) + ")";
                duplicateCount++;
            }

            std::wstring wFinalName(finalName.begin(), finalName.end());
            
            modEnabledMap[wFinalName] = false;
            
            modEnvironments.push_back(modEnv);
            loadedModNames.push_back(wFinalName);

            printf("[ModLoader] Registered: %s by %s\n", finalName.c_str(), author->c_str());
        }
    }
}

void ModManager::update() {
    std::lock_guard<std::mutex> lock(luaLock);
    
    for (size_t i = 0; i < modEnvironments.size(); ++i) {
        const std::wstring& modName = loadedModNames[i];
        if (!modEnabledMap[modName]) continue;

        sol::protected_function onTick = modEnvironments[i]["on_tick"];
        if (onTick.valid()) {
            auto result = onTick();
        }
    }
}