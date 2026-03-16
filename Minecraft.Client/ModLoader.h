#pragma once
#include "../../../lua/sol.hpp"
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <map>

class Minecraft;

class ModManager {
public:
    ModManager(Minecraft* mc);
    void loadMods(const std::string& directory);
    void update(); 
    std::map<std::wstring, bool> modEnabledMap;

    void toggleMod(const std::wstring& name) {
        modEnabledMap[name] = !modEnabledMap[name];
    }

    template<typename... Args>
    void ModManager::broadcastEvent(const std::string& functionName, Args&&... args) {
        std::lock_guard<std::mutex> lock(luaLock);

        for (size_t i = 0; i < modEnvironments.size(); ++i) {
            const std::wstring& modName = loadedModNames[i];
            
            if (!modEnabledMap[modName]) continue;

            sol::environment& env = modEnvironments[i];
            sol::protected_function func = env[functionName];

            if (func.valid()) {
                auto result = func.call(std::forward<Args>(args)...);

                if (!result.valid()) {
                    sol::error err = result;
                    
                    fprintf(stderr, "[ModLoader] [ERROR] '%ls' failed in %s(...) disabling mod...\n", 
                            modName.c_str(), functionName.c_str());
                    fprintf(stderr, "  Detail: %s\n", err.what());
                    modEnabledMap[modName] = false;
                }
            }
        }
    }

    std::vector<std::wstring> loadedModNames;

private:
    class Minecraft* minecraft;
    sol::state lua;
    std::mutex luaLock;
    
    std::vector<sol::environment> modEnvironments;
};