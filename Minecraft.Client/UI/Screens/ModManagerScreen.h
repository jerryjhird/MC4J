#pragma once
#include "../Screen.h"
#include <string>

class ModManagerScreen : public Screen {
private:
    Screen* lastScreen;
    std::wstring modsListText;

    void drawMultiLineString(std::wstring text, int x, int y, int color);

public:
    ModManagerScreen(Screen* lastScreen);
    virtual void init() override;
    virtual void render(int xm, int ym, float a) override;
    virtual void buttonClicked(Button* button) override;
    void refreshModsList();
};