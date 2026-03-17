#include "AdventureMode.h"
#include "../../Minecraft.World/Headers/net.minecraft.world.entity.player.h"

AdventureMode::AdventureMode(Minecraft *minecraft) : GameMode(minecraft)
{
    instaBuild = false; 
}

void AdventureMode::adjustPlayer(std::shared_ptr<Player> player)
{
    player->abilities.mayfly = false;
    player->abilities.flying = false;
    player->abilities.instabuild = false;
    player->abilities.invulnerable = false;
}

bool AdventureMode::destroyBlock(int x, int y, int z, int face)
{
    return false; 
}

bool AdventureMode::canHurtPlayer()
{
    return true; 
}