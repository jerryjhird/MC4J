#pragma once
#include "../../../lua/sol.hpp"
#include <mutex>
#include <vector>
#include <string>
#include <iostream>

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
    void broadcastEvent(const std::string& eventName, Args&&... args) {
        std::lock_guard<std::mutex> lock(luaLock);
        
        for (size_t i = 0; i < modEnvironments.size(); ++i) {
            std::wstring& name = loadedModNames[i];

            if (modEnabledMap.count(name) && !modEnabledMap[name]) {
                continue; 
            }

            auto& env = modEnvironments[i];
            sol::optional<sol::function> func = env[eventName];
            if (func && func->is<sol::function>()) {
                auto result = (*func)(std::forward<Args>(args)...);
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "[Lua] [Event ERR] (" << std::string(name.begin(), name.end()) 
                            << ") " << eventName << ": " << err.what() << std::endl;
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
    std::vector<sol::function> tickCallbacks;
};