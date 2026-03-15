#include "../../Platform/stdafx.h"
#include "../../Util/Class.h"
#include "../../Entities/Mobs/Painting.h"
#include "../../Platform/System.h"
#include "../../Entities/Entity.h"
#include "../../Headers/net.minecraft.world.entity.animal.h"
#include "../../Headers/net.minecraft.world.entity.item.h"
#include "../../Headers/net.minecraft.world.entity.monster.h"
#include "../../Headers/net.minecraft.world.entity.projectile.h"
#include "../../Headers/net.minecraft.world.entity.boss.enderdragon.h"
#include "../../Headers/net.minecraft.world.entity.npc.h"
#include "../../Headers/net.minecraft.world.entity.h"
#include "../../Headers/net.minecraft.world.level.h"
#include "../../Headers/com.mojang.nbt.h"
#include "EntityIO.h"

static std::unordered_map<std::wstring, entityCreateFn> _idCreateMap;
static std::unordered_map<eINSTANCEOF, std::wstring, eINSTANCEOFKeyHash, eINSTANCEOFKeyEq> _classIdMap;
static std::unordered_map<int, entityCreateFn> _numCreateMap;
static std::unordered_map<int, eINSTANCEOF> _numClassMap;
static std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash, eINSTANCEOFKeyEq> _classNumMap;
static std::unordered_map<std::wstring, int> _idNumMap;
static std::unordered_map<int, EntityIO::SpawnableMobInfo *> _idsSpawnableInCreative;

std::unordered_map<std::wstring, entityCreateFn> *EntityIO::idCreateMap = &_idCreateMap;
std::unordered_map<eINSTANCEOF, std::wstring, eINSTANCEOFKeyHash, eINSTANCEOFKeyEq> *EntityIO::classIdMap = &_classIdMap;
std::unordered_map<int, entityCreateFn> *EntityIO::numCreateMap = &_numCreateMap;
std::unordered_map<int, eINSTANCEOF> *EntityIO::numClassMap = &_numClassMap;
std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash, eINSTANCEOFKeyEq> *EntityIO::classNumMap = &_classNumMap;
std::unordered_map<std::wstring, int> *EntityIO::idNumMap = &_idNumMap;
std::unordered_map<int, EntityIO::SpawnableMobInfo *> EntityIO::idsSpawnableInCreative;

void EntityIO::setId(entityCreateFn createFn, eINSTANCEOF clas, const std::wstring &id, int idNum)
{
    idCreateMap->emplace(id, createFn);
    classIdMap->emplace(clas, id);
    numCreateMap->emplace(idNum, createFn);
    numClassMap->emplace(idNum, clas);
    classNumMap->emplace(clas, idNum);
    idNumMap->emplace(id, idNum);
}

void EntityIO::setId(entityCreateFn createFn, eINSTANCEOF clas, const std::wstring &id, int idNum, eMinecraftColour color1, eMinecraftColour color2, int nameId)
{
    setId(createFn, clas, id, idNum);
    idsSpawnableInCreative.emplace(idNum, new SpawnableMobInfo(idNum, color1, color2, nameId));
}

void EntityIO::staticCtor()
{
    _idCreateMap.reserve(128);
    _idNumMap.reserve(128);
    _numCreateMap.reserve(128);

    setId(ItemEntity::create, eTYPE_ITEMENTITY,  L"Item", 1);
    setId(ExperienceOrb::create, eTYPE_EXPERIENCEORB, L"XPOrb", 2);
    setId(Painting::create, eTYPE_PAINTING, L"Painting", 9);
    setId(Arrow::create, eTYPE_ARROW, L"Arrow", 10);
    setId(Snowball::create, eTYPE_SNOWBALL, L"Snowball", 11);
    setId(Fireball::create, eTYPE_FIREBALL, L"Fireball", 12);
    setId(SmallFireball::create, eTYPE_SMALL_FIREBALL, L"SmallFireball", 13);
    setId(ThrownEnderpearl::create, eTYPE_THROWNENDERPEARL, L"ThrownEnderpearl", 14);
    setId(EyeOfEnderSignal::create, eTYPE_EYEOFENDERSIGNAL, L"EyeOfEnderSignal", 15);
    setId(ThrownPotion::create, eTYPE_THROWNPOTION, L"ThrownPotion", 16);
    setId(ThrownExpBottle::create, eTYPE_THROWNEXPBOTTLE, L"ThrownExpBottle", 17);
    setId(ItemFrame::create, eTYPE_ITEM_FRAME, L"ItemFrame", 18);
    setId(PrimedTnt::create, eTYPE_PRIMEDTNT, L"PrimedTnt", 20);
    setId(FallingTile::create, eTYPE_FALLINGTILE, L"FallingSand", 21);
    setId(Minecart::create, eTYPE_MINECART, L"Minecart", 40);
    setId(Boat::create, eTYPE_BOAT, L"Boat", 41);
    setId(Mob::create, eTYPE_MOB, L"Mob", 48);
    setId(Monster::create, eTYPE_MONSTER, L"Monster", 49);

    setId(Creeper::create, eTYPE_CREEPER, L"Creeper", 50, eMinecraftColour_Mob_Creeper_Colour1, eMinecraftColour_Mob_Creeper_Colour2, IDS_CREEPER);
    setId(Skeleton::create, eTYPE_SKELETON, L"Skeleton", 51, eMinecraftColour_Mob_Skeleton_Colour1, eMinecraftColour_Mob_Skeleton_Colour2, IDS_SKELETON);
    setId(Spider::create, eTYPE_SPIDER, L"Spider", 52, eMinecraftColour_Mob_Spider_Colour1, eMinecraftColour_Mob_Spider_Colour2, IDS_SPIDER);
    setId(Giant::create, eTYPE_GIANT, L"Giant", 53);
    setId(Zombie::create, eTYPE_ZOMBIE, L"Zombie", 54, eMinecraftColour_Mob_Zombie_Colour1, eMinecraftColour_Mob_Zombie_Colour2, IDS_ZOMBIE);
    setId(Slime::create, eTYPE_SLIME, L"Slime", 55, eMinecraftColour_Mob_Slime_Colour1, eMinecraftColour_Mob_Slime_Colour2, IDS_SLIME);
    setId(Ghast::create, eTYPE_GHAST, L"Ghast", 56, eMinecraftColour_Mob_Ghast_Colour1, eMinecraftColour_Mob_Ghast_Colour2, IDS_GHAST);
    setId(PigZombie::create, eTYPE_PIGZOMBIE, L"PigZombie", 57, eMinecraftColour_Mob_PigZombie_Colour1, eMinecraftColour_Mob_PigZombie_Colour2, IDS_PIGZOMBIE);
    setId(EnderMan::create, eTYPE_ENDERMAN, L"Enderman", 58, eMinecraftColour_Mob_Enderman_Colour1, eMinecraftColour_Mob_Enderman_Colour2, IDS_ENDERMAN);
    setId(CaveSpider::create, eTYPE_CAVESPIDER, L"CaveSpider", 59, eMinecraftColour_Mob_CaveSpider_Colour1, eMinecraftColour_Mob_CaveSpider_Colour2, IDS_CAVE_SPIDER);
    setId(Silverfish::create, eTYPE_SILVERFISH, L"Silverfish", 60, eMinecraftColour_Mob_Silverfish_Colour1, eMinecraftColour_Mob_Silverfish_Colour2, IDS_SILVERFISH);
    setId(Blaze::create, eTYPE_BLAZE, L"Blaze", 61, eMinecraftColour_Mob_Blaze_Colour1, eMinecraftColour_Mob_Blaze_Colour2, IDS_BLAZE);
    setId(LavaSlime::create, eTYPE_LAVASLIME, L"LavaSlime", 62, eMinecraftColour_Mob_LavaSlime_Colour1, eMinecraftColour_Mob_LavaSlime_Colour2, IDS_LAVA_SLIME);
    setId(EnderDragon::create, eTYPE_ENDERDRAGON, L"EnderDragon", 63);

    setId(Pig::create, eTYPE_PIG, L"Pig", 90, eMinecraftColour_Mob_Pig_Colour1, eMinecraftColour_Mob_Pig_Colour2, IDS_PIG);
    setId(Sheep::create, eTYPE_SHEEP, L"Sheep", 91, eMinecraftColour_Mob_Sheep_Colour1, eMinecraftColour_Mob_Sheep_Colour2, IDS_SHEEP);
    setId(Cow::create, eTYPE_COW, L"Cow", 92, eMinecraftColour_Mob_Cow_Colour1, eMinecraftColour_Mob_Cow_Colour2, IDS_COW);
    setId(Chicken::create, eTYPE_CHICKEN, L"Chicken", 93, eMinecraftColour_Mob_Chicken_Colour1, eMinecraftColour_Mob_Chicken_Colour2, IDS_CHICKEN);
    setId(Squid::create, eTYPE_SQUID, L"Squid", 94, eMinecraftColour_Mob_Squid_Colour1, eMinecraftColour_Mob_Squid_Colour2, IDS_SQUID);
    setId(Wolf::create, eTYPE_WOLF, L"Wolf", 95, eMinecraftColour_Mob_Wolf_Colour1, eMinecraftColour_Mob_Wolf_Colour2, IDS_WOLF);
    setId(MushroomCow::create, eTYPE_MUSHROOMCOW, L"MushroomCow", 96, eMinecraftColour_Mob_MushroomCow_Colour1, eMinecraftColour_Mob_MushroomCow_Colour2, IDS_MUSHROOM_COW);
    setId(SnowMan::create, eTYPE_SNOWMAN, L"SnowMan", 97);
    setId(Ozelot::create, eTYPE_OZELOT, L"Ozelot", 98, eMinecraftColour_Mob_Ocelot_Colour1, eMinecraftColour_Mob_Ocelot_Colour2, IDS_OZELOT);
    setId(VillagerGolem::create, eTYPE_VILLAGERGOLEM, L"VillagerGolem", 99);

    setId(Villager::create, eTYPE_VILLAGER, L"Villager", 120, eMinecraftColour_Mob_Villager_Colour1, eMinecraftColour_Mob_Villager_Colour2, IDS_VILLAGER);
    setId(EnderCrystal::create, eTYPE_ENDER_CRYSTAL, L"EnderCrystal", 200);

    // 4J Added
    setId(DragonFireball::create, eTYPE_DRAGON_FIREBALL, L"DragonFireball", 1000);
}

std::shared_ptr<Entity> EntityIO::newEntity(const std::wstring& id, Level *level)
{
    auto it = idCreateMap->find(id);
    if(it != idCreateMap->end())
    {
        auto create = it->second;
        if (create != nullptr) return std::shared_ptr<Entity>(create(level));
    }
    return nullptr;
}

std::shared_ptr<Entity> EntityIO::loadStatic(CompoundTag *tag, Level *level)
{
    auto entity = newEntity(tag->getString(L"id"), level);
    if (entity != nullptr) entity->load(tag);
#ifdef _DEBUG
    else app.DebugPrintf("Skipping Entity with id %ls\n", tag->getString(L"id").c_str());
#endif
    return entity;
}

std::shared_ptr<Entity> EntityIO::newById(int id, Level *level)
{
    auto it = numCreateMap->find(id);
    if(it != numCreateMap->end() && it->second != nullptr)
    {
        return std::shared_ptr<Entity>(it->second(level));
    }
    return nullptr;
}

std::shared_ptr<Entity> EntityIO::newByEnumType(eINSTANCEOF eType, Level *level)
{
    auto it = classNumMap->find(eType);
    if(it != classNumMap->end())
    {
        auto it2 = numCreateMap->find(it->second);
        if(it2 != numCreateMap->end() && it2->second != nullptr)
        {
            return std::shared_ptr<Entity>(it2->second(level));
        }
    }
    return nullptr;
}

int EntityIO::getId(std::shared_ptr<Entity> entity)
{
    auto it = classNumMap->find(entity->GetType());
    return (it != classNumMap->end()) ? it->second : -1;
}

std::wstring EntityIO::getEncodeId(std::shared_ptr<Entity> entity)
{
    auto it = classIdMap->find(entity->GetType());
    return (it != classIdMap->end()) ? it->second : L"";
}

int EntityIO::getId(const std::wstring &encodeId)
{
    auto it = idNumMap->find(encodeId);
    return (it == idNumMap->end()) ? 90 : it->second; // Default to Pig
}

std::wstring EntityIO::getEncodeId(int entityIoValue)
{
    auto it = numClassMap->find(entityIoValue);
    if(it != numClassMap->end())
    {
        auto classIdIt = classIdMap->find(it->second);
        if(classIdIt != classIdMap->end()) return classIdIt->second;
    }
    return L"";
}

int EntityIO::getNameId(int entityIoValue)
{
    auto it = idsSpawnableInCreative.find(entityIoValue);
    return (it != idsSpawnableInCreative.end()) ? it->second->nameId : -1;
}

eINSTANCEOF EntityIO::getType(const std::wstring &idString)
{
    auto it = numClassMap->find(getId(idString));
    return (it != numClassMap->end()) ? it->second : eTYPE_NOTSET;
}

eINSTANCEOF EntityIO::getClass(int id)
{
    auto it = numClassMap->find(id);
    return (it != numClassMap->end()) ? it->second : eTYPE_NOTSET;
}

int EntityIO::eTypeToIoid(eINSTANCEOF eType)
{
    auto it = classNumMap->find(eType);
    return (it != classNumMap->end()) ? it->second : -1;
}