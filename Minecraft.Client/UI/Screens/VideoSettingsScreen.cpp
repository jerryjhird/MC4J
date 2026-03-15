#include "../../Platform/stdafx.h"
#include "VideoSettingsScreen.h"
#include "../SmallButton.h"
#include "../SlideButton.h"
#include "../../GameState/Options.h"
#include "ControlsScreen.h"
#include "../../../Minecraft.World/Headers/net.minecraft.locale.h"

VideoSettingsScreen::VideoSettingsScreen(Screen *lastScreen, Options *options)
{
	this->title = L"Video Settings"; // 4J - added
    this->lastScreen = lastScreen;
    this->options = options;
}

void VideoSettingsScreen::init()
{
    Language *language = Language::getInstance();
    this->title = language->getElement(L"options.videoTitle");

    // We've expanded this to 10 items to include the "Hidden" ones
    const Options::Option *items[10] = {
        Options::Option::GRAPHICS,
        Options::Option::RENDER_DISTANCE,
        Options::Option::AMBIENT_OCCLUSION,
        Options::Option::FRAMERATE_LIMIT,
        Options::Option::ANAGLYPH,          
        Options::Option::VIEW_BOBBING,
        Options::Option::GUI_SCALE,
        Options::Option::ADVANCED_OPENGL,
        Options::Option::GAMMA,
        Options::Option::FOV
    };

    for (int position = 0; position < 10; position++)
    {
        const Options::Option *item = items[position];
        int xPos = width / 2 - 155 + (position % 2 * 160);
        int yPos = height / 6 + 24 * (position / 2);

        if (!item->isProgress())
        {
            buttons.push_back(new SmallButton(item->getId(), xPos, yPos, item, options->getMessage(item)));
        }
        else
        {
            buttons.push_back(new SlideButton(item->getId(), xPos, yPos, item, options->getMessage(item), options->getProgressValue(item)));
        }
    }

//        buttons.add(new Button(VIDEO_BUTTON_ID, width / 2 - 100, height / 6 + 24 * 4 + 12, language.getElement("options.video")));
//        buttons.add(new Button(CONTROLS_BUTTON_ID, width / 2 - 100, height / 6 + 24 * 5 + 12, language.getElement("options.controls")));
    buttons.push_back(new Button(200, width / 2 - 100, height / 6 + 24 * 6, language->getElement(L"gui.done")));

}

void VideoSettingsScreen::buttonClicked(Button *button)
{
    if (!button->active) return;
    if (button->id < 100 && (dynamic_cast<SmallButton *>(button) != NULL))
	{
        options->toggle(((SmallButton *) button)->getOption(), 1);
        button->msg = options->getMessage(Options::Option::getItem(button->id));
    }
    if (button->id == 200)
	{
        minecraft->options->save();
        minecraft->setScreen(lastScreen);
    }

    ScreenSizeCalculator ssc(minecraft->options, minecraft->width, minecraft->height);
    int screenWidth = ssc.getWidth();
    int screenHeight = ssc.getHeight();
    Screen::init(minecraft, screenWidth, screenHeight);	// 4J - was this.init
}

void VideoSettingsScreen::render(int xm, int ym, float a)
{
    renderBackground();
    drawCenteredString(font, title, width / 2, 20, 0xffffff);
    Screen::render(xm, ym, a);
}
