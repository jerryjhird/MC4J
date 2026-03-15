#include "../../Platform/stdafx.h"
#include "ModManagerScreen.h"
#include "../Button.h"
#include "../../../Minecraft.World/Headers/net.minecraft.locale.h"
#include "../../ModLoader.h"

ModManagerScreen::ModManagerScreen(Screen *lastScreen) {
    this->lastScreen = lastScreen;
}

void ModManagerScreen::init() {
    Language *language = Language::getInstance();
    buttons.clear();
    refreshModsList();

    if (minecraft && minecraft->modManager) {
        int startY = 80;
        int index = 0;
        
        for (const auto& name : minecraft->modManager->loadedModNames) {
            bool isEnabled = minecraft->modManager->modEnabledMap[name];
            
            std::wstring btnText = isEnabled ? L"ENABLED" : L"DISABLED";

            buttons.push_back(new Button(100 + index, width / 2 - 50, startY + (index * 24), 100, 20, btnText));
            index++;
        }
    }

    buttons.push_back(new Button(0, width / 2 - 100, height - 52, L"Refresh Mods"));
    buttons.push_back(new Button(1, width / 2 - 100, height - 28, language->getElement(L"gui.cancel")));
}

void ModManagerScreen::buttonClicked(Button *button) {
    if (!button || !button->active || !minecraft || !minecraft->modManager) {
        return;
    }

    if (button->id == 1) { 
        minecraft->setScreen(lastScreen);
    }
    
    else if (button->id == 0) {
        const char* env = std::getenv("MODS");
        std::string path = env ? env : "./Mods";
        
        minecraft->modManager->loadMods(path);
        
        this->init();
    }

    else if (button->id >= 100) {
        size_t modIndex = static_cast<size_t>(button->id - 100);
        
        if (modIndex < minecraft->modManager->loadedModNames.size()) {
            std::wstring modName = minecraft->modManager->loadedModNames[modIndex];
            
            minecraft->modManager->toggleMod(modName);
            
            this->init();
        }
    }
}

void ModManagerScreen::drawMultiLineString(std::wstring text, int x, int y, int color) {
    size_t last = 0;
    size_t next = 0;
    int currentY = y;

    while ((next = text.find(L'\n', last)) != std::wstring::npos) {
        drawCenteredString(font, text.substr(last, next - last), x, currentY, color);
        currentY += 12; 
        last = next + 1;
    }
    drawCenteredString(font, text.substr(last), x, currentY, color);
}

void ModManagerScreen::render(int xm, int ym, float a) {
    renderBackground();
    drawCenteredString(font, L"Jerry's Mod Manager", width / 2, 20, 0xffffff);

    if (minecraft && minecraft->modManager) {
        int startY = 80;
        int index = 0;
        
        for (const auto& name : minecraft->modManager->loadedModNames) {
            int currentY = startY + (index * 24) + 5;
            
            drawString(font, name, 40, currentY, 0xFFFFFF);
            
            index++;
        }
    }

    drawString(font, ClientConstants::VERSION_STRING, 2, 2, 0x505050);
    Screen::render(xm, ym, a);
}

void ModManagerScreen::refreshModsList() {
    if (!minecraft) {
        modsListText = L"Searching for Minecraft instance...";
        return;
    }

    if (minecraft->modManager) {
        const char* env = std::getenv("MODS");
        std::string path = env ? env : "./Mods";

        modsListText = L"Mods:\n";
        if (minecraft->modManager->loadedModNames.empty()) {
            modsListText += L"(No mods found in " + std::wstring(path.begin(), path.end()) + L")";
        } else {
            for (const auto& name : minecraft->modManager->loadedModNames) {
                modsListText += L"- " + name + L"\n";
            }
        }
    } else {
        modsListText = L"Waiting for ModManager to initialize...";
    }
}