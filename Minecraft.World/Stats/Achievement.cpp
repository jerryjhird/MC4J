#include "../Platform/stdafx.h"
#include "../Headers/net.minecraft.locale.h"
#include "../Items/ItemInstance.h"
#include "Achievements.h"
#include "../Util/DescFormatter.h"
#include "Achievement.h"

void Achievement::_init()
{
	isGoldenVar = false;

	if (x < Achievements::xMin) Achievements::xMin = x;
	if (y < Achievements::yMin) Achievements::yMin = y;
	if (x > Achievements::xMax) Achievements::xMax = x;
	if (y > Achievements::yMax) Achievements::yMax = y;
}

Achievement::Achievement(int id, const std::wstring& name, int x, int y, Item *icon, Achievement *requires)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id, L"achievement." + name), 
      desc(L"achievement." + name + L".desc"), 
      x(x), y(y), icon(new ItemInstance(icon)), requires(requires)
{
}

Achievement::Achievement(int id, const std::wstring& name, int x, int y, Tile *icon, Achievement *requires)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id, L"achievement." + name), 
      desc(L"achievement." + name + L".desc"), 
      x(x), y(y), icon(new ItemInstance(icon)), requires(requires)
{
}

Achievement::Achievement(int id, const std::wstring& name, int x, int y, std::shared_ptr<ItemInstance> icon, Achievement *requires)
    : Stat(Achievements::ACHIEVEMENT_OFFSET + id, L"achievement." + name), 
      desc(L"achievement." + name + L".desc"), 
      x(x), y(y), icon(icon), requires(requires)
{
}

Achievement *Achievement::setAwardLocallyOnly()
{
	awardLocallyOnly = true;
	return this;
}

Achievement *Achievement::setGolden() 
{
	isGoldenVar = true;
	return this;
}

Achievement *Achievement::postConstruct()
{
	Stat::postConstruct();

	Achievements::achievements->push_back(this);

	return this;
}

bool Achievement::isAchievement() 
{
	return true;
}

std::wstring Achievement::getDescription() {
    return I18n::get(this->desc);
}

Achievement *Achievement::setDescFormatter(DescFormatter *descFormatter)
{
	this->descFormatter = descFormatter;
	return this;
}

bool Achievement::isGolden()
{
	return isGoldenVar;
}

int Achievement::getAchievementID()
{
	return id - Achievements::ACHIEVEMENT_OFFSET;
}
