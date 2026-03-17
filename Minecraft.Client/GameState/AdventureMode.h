#pragma once
#include "GameMode.h"

class AdventureMode : public GameMode
{
public:
    AdventureMode(Minecraft *minecraft);

    virtual bool destroyBlock(int x, int y, int z, int face) override;
    virtual bool canHurtPlayer() override;
    virtual void adjustPlayer(std::shared_ptr<Player> player) override;

    virtual bool hasMissTime() override { return true; }
    virtual bool hasInfiniteItems() override { return false; }
    virtual bool hasFarPickRange() override { return false; }
};