#include "../Platform/stdafx.h"
#include "Chunk.h"
#include "EntityRenderers/TileRenderer.h"
#include "EntityRenderers/TileEntityRenderDispatcher.h"
#include "../../Minecraft.World/Headers/net.minecraft.world.level.h"
#include "../../Minecraft.World/Headers/net.minecraft.world.level.chunk.h"
#include "../../Minecraft.World/Headers/net.minecraft.world.level.tile.h"
#include "../../Minecraft.World/Headers/net.minecraft.world.level.tile.entity.h"
#include "LevelRenderer.h"

#ifdef __PS3__
#include "../Platform/PS3/SPU_Tasks/ChunkUpdate/ChunkRebuildData.h"
#include "../Platform/PS3/SPU_Tasks/ChunkUpdate/TileRenderer_SPU.h"
#include "../Platform/PS3/SPU_Tasks/CompressedTile/CompressedTileStorage_SPU.h"

#include "../Platform/PS3/PS3Extras/C4JThread_SPU.h"
#include "../Platform/PS3/PS3Extras/C4JSpursJob.h"
#endif

int Chunk::updates = 0;

#ifdef _LARGE_WORLDS
unsigned int Chunk::tlsIdx = TlsAlloc();

void Chunk::CreateNewThreadStorage()
{
	unsigned char *tileIds = new unsigned char[16 * 16 * Level::maxBuildHeight];
	TlsSetValue(tlsIdx, tileIds);
}

void Chunk::ReleaseThreadStorage()
{
	unsigned char *tileIds = (unsigned char *)TlsGetValue(tlsIdx);
	delete tileIds;
}

unsigned char *Chunk::GetTileIdsStorage()
{
	unsigned char *tileIds = (unsigned char *)TlsGetValue(tlsIdx);
	return tileIds;
}
#else
// 4J Stu - Don't want this when multi-threaded
Tesselator *Chunk::t = Tesselator::getInstance();
#endif
LevelRenderer *Chunk::levelRenderer;

// TODO - 4J see how input entity vector is set up and decide what way is best to pass this to the function
Chunk::Chunk(Level *level, LevelRenderer::rteMap &globalRenderableTileEntities, CRITICAL_SECTION& globalRenderableTileEntities_cs, int x, int y, int z, ClipChunk *clipChunk)
	: globalRenderableTileEntities( &globalRenderableTileEntities ), globalRenderableTileEntities_cs(&globalRenderableTileEntities_cs)
{
	clipChunk->visible = false;
	bb = NULL;
	id = 0;

	this->level = level;
	//this->globalRenderableTileEntities = globalRenderableTileEntities;

	assigned = false;
	this->clipChunk = clipChunk;
	setPos(x, y, z);
}

void Chunk::setPos(int x, int y, int z)
{
	if(assigned && (x == this->x && y == this->y && z == this->z)) return;

	reset();

	this->x = x;
	this->y = y;
	this->z = z;
	xm = x + XZSIZE / 2;
	ym = y + SIZE / 2;
	zm = z + XZSIZE / 2;
	clipChunk->xm = xm;
	clipChunk->ym = ym;
	clipChunk->zm = zm;

	clipChunk->globalIdx = LevelRenderer::getGlobalIndexForChunk(x, y, z, level);

#if 1
	// 4J - we're not using offsetted renderlists anymore, so just set the full position of this chunk into x/y/zRenderOffs where
	// it will be used directly in the renderlist of this chunk
	xRenderOffs = x;
	yRenderOffs = y;
	zRenderOffs = z;
	xRender = 0;
	yRender = 0;
	zRender = 0;
#else
	xRenderOffs = x & 1023;
	yRenderOffs = y;
	zRenderOffs = z & 1023;
	xRender = x - xRenderOffs;
	yRender = y - yRenderOffs;
	zRender = z - zRenderOffs;
#endif

	float g = 6.0f;
	// 4J - changed to just set the value rather than make a new one, if we've already created storage
	if( bb == NULL )
	{
		bb = AABB::newPermanent(-g, -g, -g, XZSIZE+g, SIZE+g, XZSIZE+g);
	}
 	else 
 	{
		// 4J MGH - bounds are relative to the position now, so the AABB will be setup already, either above, or from the tesselator bounds.
// 		bb->set(-g, -g, -g, SIZE+g, SIZE+g, SIZE+g);
 	}
	clipChunk->aabb[0] = bb->x0 + x;
	clipChunk->aabb[1] = bb->y0 + y;
	clipChunk->aabb[2] = bb->z0 + z;
	clipChunk->aabb[3] = bb->x1 + x;
	clipChunk->aabb[4] = bb->y1 + y;
	clipChunk->aabb[5] = bb->z1 + z;

	assigned = true;

	EnterCriticalSection(&levelRenderer->m_csDirtyChunks);
	unsigned char refCount = levelRenderer->incGlobalChunkRefCount(x, y, z, level);
//	printf("\t\t [inc] refcount %d at %d, %d, %d\n",refCount,x,y,z);

//	int idx = levelRenderer->getGlobalIndexForChunk(x, y, z, level);

	// If we're the first thing to be referencing this, mark it up as dirty to get rebuilt
	if( refCount == 1 )
	{
//		printf("Setting %d %d %d dirty [%d]\n",x,y,z, idx);
		// Chunks being made dirty in this way can be very numerous (eg the full visible area of the world at start up, or a whole edge of the world when moving).
		// On account of this, don't want to stick them into our lock free queue that we would normally use for letting the render update thread know about this chunk.
		// Instead, just set the flag to say this is dirty, and then pass a special value of 1 through to the lock free stack which lets that thread know that at least
		// one chunk other than the ones in the stack itself have been made dirty.
		levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_DIRTY );
#ifdef _XBOX
		PIXSetMarker(0,"Non-stack event pushed");
#else
		PIXSetMarkerDeprecated(0,"Non-stack event pushed");
#endif
	}

	LeaveCriticalSection(&levelRenderer->m_csDirtyChunks);

	
}

void Chunk::translateToPos()
{
	glTranslatef((float)xRenderOffs, (float)yRenderOffs, (float)zRenderOffs);
}


Chunk::Chunk()
{
}

void Chunk::makeCopyForRebuild(Chunk *source)
{
	this->level = source->level;
	this->x = source->x;
	this->y = source->y;
	this->z = source->z;
	this->xRender = source->xRender;
	this->yRender = source->yRender;
	this->zRender = source->zRender;
	this->xRenderOffs = source->xRenderOffs;
	this->yRenderOffs = source->yRenderOffs;
	this->zRenderOffs = source->zRenderOffs;
	this->xm = source->xm;
	this->ym = source->ym;
	this->zm = source->zm;
	this->bb = source->bb;
	this->clipChunk = NULL;
	this->id = source->id;
	this->globalRenderableTileEntities = source->globalRenderableTileEntities;
	this->globalRenderableTileEntities_cs = source->globalRenderableTileEntities_cs;	
}

void Chunk::rebuild()
{
    PIXBeginNamedEvent(0, "Rebuilding chunk %d, %d, %d", x, y, z);
    PIXBeginNamedEvent(0, "Rebuild section A");
    
	#ifdef _LARGE_WORLDS
		Tesselator *t = Tesselator::getInstance();
	#else
		Chunk::t = Tesselator::getInstance();
	#endif

    updates++;

    int x0 = x, y0 = y, z0 = z;
    int x1 = x + XZSIZE, y1 = y + SIZE, z1 = z + XZSIZE;

    LevelChunk::touchedSky = false;

    std::vector<std::shared_ptr<TileEntity>> renderableTileEntities;

    int r = 1;
    int lists = levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level) * 2;
    lists += levelRenderer->chunkLists;

    PIXEndNamedEvent();
    PIXBeginNamedEvent(0, "Rebuild section B");
	#ifdef _LARGE_WORLDS
		unsigned char *tileIds = GetTileIdsStorage();
	#else
		static unsigned char tileIds[16 * 16 * Level::maxBuildHeight];
	#endif
    
    byteArray tileArray(tileIds, 16 * 16 * Level::maxBuildHeight);
    level->getChunkAt(x, z)->getBlockData(tileArray);

    Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
    TileRenderer tileRenderer(&region, this->x, this->y, this->z, tileIds);

    auto getTileRef = [&](int lx, int ly, int lz) -> unsigned char& {
        int indexY = ly;
        int offset = 0;
        if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
            indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
            offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
        }
        return tileIds[offset + (lx << 11) + (lz << 7) + indexY];
    };

    auto isCullable = [](unsigned char id) {
        return (id == Tile::rock_Id || id == Tile::dirt_Id || id == Tile::unbreakable_Id || id == 255);
    };

    bool empty = true;
    for (int yy = y0; yy < y1; yy++)
    {
        for (int zz = 0; zz < 16; zz++)
        {
            for (int xx = 0; xx < 16; xx++)
            {
                unsigned char& tileId = getTileRef(xx, yy, zz);
                if (tileId > 0) empty = false;

                if (yy == (Level::maxBuildHeight - 1)) continue;
                if ((xx == 0) || (xx == 15)) continue;
                if ((zz == 0) || (zz == 15)) continue;

                if (!isCullable(tileId)) continue;
                if (!isCullable(getTileRef(xx - 1, yy, zz))) continue;
                if (!isCullable(getTileRef(xx + 1, yy, zz))) continue;
                if (!isCullable(getTileRef(xx, yy, zz - 1))) continue;
                if (!isCullable(getTileRef(xx, yy, zz + 1))) continue;
                
                if (yy > 0) {
                    if (!isCullable(getTileRef(xx, yy - 1, zz))) continue;
                }
                
                if (!isCullable(getTileRef(xx, yy + 1, zz))) continue;

                tileId = 0xff;
            }
        }
    }
    PIXEndNamedEvent();

    if (empty)
    {
        for (int currentLayer = 0; currentLayer < 2; currentLayer++) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
            RenderManager.CBuffClear(lists + currentLayer);
        }
        return;
    }

    PIXBeginNamedEvent(0, "Rebuild section C");
    Tesselator::Bounds bounds;
    {
        float g = 6.0f;
        bounds.boundingBox[0] = -g; bounds.boundingBox[1] = -g; bounds.boundingBox[2] = -g;
        bounds.boundingBox[3] = XZSIZE + g; bounds.boundingBox[4] = SIZE + g; bounds.boundingBox[5] = XZSIZE + g;
    }

    for (int currentLayer = 0; currentLayer < 2; currentLayer++)
    {
        bool renderNextLayer = false;
        bool rendered = false;
        bool started = false;

        for (int z = z0; z < z1; z++)
        {
            for (int x = x0; x < x1; x++)
            {
                for (int y = y0; y < y1; y++)
                {
                    unsigned char tileId = getTileRef(x - x0, y, z - z0);
                    
                    if (tileId == 0xff) continue;

                    if (tileId > 0)
                    {
                        if (!started)
                        {
                            started = true;
                            MemSect(31);
                            glNewList(lists + currentLayer, GL_COMPILE);
                            MemSect(0);
                            glPushMatrix();
                            glDepthMask(true);
                            t->useCompactVertices(false);
                            translateToPos();
                            t->begin();
                            t->offset((float)(-this->x), (float)(-this->y), (float)(-this->z));
                        }

                        Tile *tile = Tile::tiles[tileId];
                        if (currentLayer == 0 && tile->isEntityTile())
                        {
                            std::shared_ptr<TileEntity> et = region.getTileEntity(x, y, z);
                            if (TileEntityRenderDispatcher::instance->hasRenderer(et)) {
                                renderableTileEntities.push_back(et);
                            }
                        }
                        
                        int renderLayer = tile->getRenderLayer();
                        if (renderLayer != currentLayer) {
                            renderNextLayer = true;
                        } else {
                            rendered |= tileRenderer.tesselateInWorld(tile, x, y, z);
                        }
                    }
                }
            }
        }
        if (started)
        {
            t->end();
            bounds.addBounds(t->bounds);
            glPopMatrix();
            glEndList();
            t->useCompactVertices(false);
            t->offset(0, 0, 0);
        }
        else rendered = false;

        if (rendered) {
            levelRenderer->clearGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
        } else {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
            RenderManager.CBuffClear(lists + currentLayer);
        }
        
        if ((currentLayer == 0) && (!renderNextLayer)) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY1);
            RenderManager.CBuffClear(lists + 1);
            break;
        }
    }

    if (bb) {
        bb->set(bounds.boundingBox[0], bounds.boundingBox[1], bounds.boundingBox[2],
                bounds.boundingBox[3], bounds.boundingBox[4], bounds.boundingBox[5]);
    }

    PIXEndNamedEvent();
    PIXBeginNamedEvent(0, "Rebuild section D");

    int key = levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level);
    EnterCriticalSection(globalRenderableTileEntities_cs);
    
    if (renderableTileEntities.size())
    {
        auto it = globalRenderableTileEntities->find(key);
        if (it != globalRenderableTileEntities->end())
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                (*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageFlaggedAtChunk);
            }

            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                auto it2 = std::find(it->second.begin(), it->second.end(), renderableTileEntities[i]);
                if (it2 == it->second.end()) {
                    (*globalRenderableTileEntities)[key].push_back(renderableTileEntities[i]);
                } else {
                    (*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageKeep);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                (*globalRenderableTileEntities)[key].push_back(renderableTileEntities[i]);
            }
        }
    }
    else
    {
        auto it = globalRenderableTileEntities->find(key);
        if (it != globalRenderableTileEntities->end()) {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                (*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageFlaggedAtChunk);
            }
        }
    }
    LeaveCriticalSection(globalRenderableTileEntities_cs);

    if (LevelChunk::touchedSky) levelRenderer->clearGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    else levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    
    levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_COMPILED);
    PIXEndNamedEvent();
}


#ifdef __PS3__
ChunkRebuildData g_rebuildDataIn __attribute__((__aligned__(16)));
ChunkRebuildData g_rebuildDataOut __attribute__((__aligned__(16)));
TileCompressData_SPU g_tileCompressDataIn __attribute__((__aligned__(16)));
unsigned char* g_tileCompressDataOut = (unsigned char*)&g_rebuildDataIn.m_tileIds;

#define TILE_RENDER_SPU


void RunSPURebuild()
{

	static C4JSpursJobQueue::Port p("C4JSpursJob_ChunkUpdate");
	C4JSpursJob_CompressedTile tileJob(&g_tileCompressDataIn,g_tileCompressDataOut);
	C4JSpursJob_ChunkUpdate chunkJob(&g_rebuildDataIn, &g_rebuildDataOut);

	if(g_rebuildDataIn.m_currentLayer == 0)		// only need to create the tiles on the first layer
	{
		p.submitJob(&tileJob);
 		p.submitSync();
	}

	p.submitJob(&chunkJob);
	p.waitForCompletion();

	assert(g_rebuildDataIn.m_x0 == g_rebuildDataOut.m_x0);
}

void Chunk::rebuild_SPU()
{

//	if (!dirty) return;
	Chunk::t = Tesselator::getInstance(); // 4J - added - static initialiser being set at the wrong time
	updates++;

	int x0 = x;
	int y0 = y;
	int z0 = z;
	int x1 = x + SIZE;
	int y1 = y + SIZE;
	int z1 = z + SIZE;

	LevelChunk::touchedSky = false;

//	std::unordered_set<std::shared_ptr<TileEntity> > oldTileEntities(renderableTileEntities.begin(),renderableTileEntities.end());		// 4J removed this & next line
//	renderableTileEntities.clear();

	std::vector<std::shared_ptr<TileEntity> > renderableTileEntities;	// 4J - added

//        List<TileEntity> newTileEntities = new ArrayList<TileEntity>();
//        newTileEntities.clear();
//        renderableTileEntities.clear();

	int r = 1;

	Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
	TileRenderer tileRenderer(&region);

	int lists = levelRenderer->getGlobalIndexForChunk(this->x,this->y,this->z,level) * 2;
	lists += levelRenderer->chunkLists;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 4J - optimisation begins.

	// Get the data for the level chunk that this render chunk is it (level chunk is 16 x 16 x 128,
	// render chunk is 16 x 16 x 16. We wouldn't have to actually get all of it if the data was ordered differently, but currently
	// it is ordered by x then z then y so just getting a small range of y out of it would involve getting the whole thing into
	// the cache anyway. 
	ChunkRebuildData* pOutData = NULL;
	g_rebuildDataIn.buildForChunk(&region, level, x0, y0, z0);

 	Tesselator::Bounds bounds;
	{
		// this was the old default clip bounds for the chunk, set in Chunk::setPos. 
		float g = 6.0f;
		bounds.boundingBox[0] = -g;
		bounds.boundingBox[1] = -g;
		bounds.boundingBox[2] = -g;
		bounds.boundingBox[3] = SIZE+g;
		bounds.boundingBox[4] = SIZE+g;
		bounds.boundingBox[5] = SIZE+g;
	}

	for (int currentLayer = 0; currentLayer < 2; currentLayer++)
	{
		bool rendered = false;

		{
			glNewList(lists + currentLayer, GL_COMPILE);
			MemSect(0);
			glPushMatrix();
			glDepthMask(true);	// 4J added
			t->useCompactVertices(false);	 // 4J added
			translateToPos();
			float ss = 1.000001f;
			// 4J - have removed this scale as I don't think we should need it, and have now optimised the vertex
			// shader so it doesn't do anything other than translate with this matrix anyway
	#if 0
			glTranslatef(-zs / 2.0f, -ys / 2.0f, -zs / 2.0f);
			glScalef(ss, ss, ss);
			glTranslatef(zs / 2.0f, ys / 2.0f, zs / 2.0f);
	#endif
			t->begin();
			t->offset((float)(-this->x), (float)(-this->y), (float)(-this->z));
		}

		g_rebuildDataIn.copyFromTesselator();
		intArray_SPU tesselatorArray((unsigned int*)g_rebuildDataIn.m_tesselator.m_PPUArray);
		g_rebuildDataIn.m_tesselator._array = &tesselatorArray;
		g_rebuildDataIn.m_currentLayer = currentLayer;
	#ifdef TILE_RENDER_SPU
		g_tileCompressDataIn.setForChunk(&region, x0, y0, z0);
		RunSPURebuild();
 		g_rebuildDataOut.storeInTesselator();
		pOutData = &g_rebuildDataOut;
	#else
		g_rebuildDataIn.disableUnseenTiles();
		TileRenderer_SPU *pTileRenderer = new TileRenderer_SPU(&g_rebuildDataIn);
		g_rebuildDataIn.tesselateAllTiles(pTileRenderer);
		g_rebuildDataIn.storeInTesselator();
		pOutData = &g_rebuildDataIn;
	#endif
 		if(pOutData->m_flags & ChunkRebuildData::e_flag_Rendered)
			rendered = true;

				// 4J - changed loop order here to leave y as the innermost loop for better cache performance
		for (int z = z0; z < z1; z++)
		{
			for (int x = x0; x < x1; x++)
			{
				for (int y = y0; y < y1; y++)
				{
					// 4J - get tile from those copied into our local array in earlier optimisation
					unsigned char tileId = pOutData->getTile(x,y,z);
					if (tileId > 0)
					{
						if (currentLayer == 0 && Tile::tiles[tileId]->isEntityTile())
						{
							std::shared_ptr<TileEntity> et = region.getTileEntity(x, y, z);
							if (TileEntityRenderDispatcher::instance->hasRenderer(et))
							{
								renderableTileEntities.push_back(et);
							}
						}
						int flags = pOutData->getFlags(x,y,z);
						if(flags & ChunkRebuildData::e_flag_SPURenderCodeMissing)
						{

							Tile *tile = Tile::tiles[tileId];
							int renderLayer = tile->getRenderLayer();

							if (renderLayer != currentLayer)
							{
	//							renderNextLayer = true;
							}
							else if (renderLayer == currentLayer)
							{
								//if(currentLayer == 0)
								//	numRenderedLayer0++;
 								rendered |= tileRenderer.tesselateInWorld(tile, x, y, z);
							}
						}
					}
				}
			}
		}


		{
			t->end();
			bounds.addBounds(t->bounds);
			glPopMatrix();
			glEndList();
			t->useCompactVertices(false);	 // 4J added
			t->offset(0, 0, 0);
		}
		if (rendered)
		{
			levelRenderer->clearGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
		}
		else
		{
			// 4J - added - clear any renderer data associated with this unused list
			levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
			RenderManager.CBuffClear(lists + currentLayer);
		}

	}

	if( bb )
	{
		bb->set(bounds.boundingBox[0], bounds.boundingBox[1], bounds.boundingBox[2],
			bounds.boundingBox[3], bounds.boundingBox[4], bounds.boundingBox[5]);
	}


	if(pOutData->m_flags & ChunkRebuildData::e_flag_TouchedSky)
		LevelChunk::touchedSky = true;


	// 4J - have rewritten the way that tile entities are stored globally to make it work more easily with split screen. Chunks are now
	// stored globally in the levelrenderer, in a hashmap with a special key made up from the dimension and chunk position (using same index
	// as is used for global flags)
#if 1
	int key = levelRenderer->getGlobalIndexForChunk(this->x,this->y,this->z,level);
	EnterCriticalSection(globalRenderableTileEntities_cs);
	if( renderableTileEntities.size() )
	{
		AUTO_VAR(it, globalRenderableTileEntities->find(key));
		if( it != globalRenderableTileEntities->end() )
		{
			// We've got some renderable tile entities that we want associated with this chunk, and an existing list of things that used to be.
			// We need to flag any that we don't need any more to be removed, keep those that we do, and add any new ones

			// First pass - flag everything already existing to be removed
			for( AUTO_VAR(it2, it->second.begin()); it2 != it->second.end(); it2++ )
			{
				(*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageFlaggedAtChunk);
			}

			// Now go through the current list. If these are already in the list, then unflag the remove flag. If they aren't, then add
			for( int i = 0; i < renderableTileEntities.size(); i++ )
			{
				AUTO_VAR(it2, find( it->second.begin(), it->second.end(), renderableTileEntities[i] ));
				if( it2 == it->second.end() )
				{
					(*globalRenderableTileEntities)[key].push_back(renderableTileEntities[i]);
				}
				else
				{
					(*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageKeep);
				}
			}
		}
		else
		{
			// Easy case - nothing already existing for this chunk. Add them all in.
			for( int i = 0; i < renderableTileEntities.size(); i++ )
			{
				(*globalRenderableTileEntities)[key].push_back(renderableTileEntities[i]);
			}
		}
	}
	else
	{
		// Another easy case - we don't want any renderable tile entities associated with this chunk. Flag all to be removed.
		AUTO_VAR(it, globalRenderableTileEntities->find(key));
		if( it != globalRenderableTileEntities->end() )
		{
			for( AUTO_VAR(it2, it->second.begin()); it2 != it->second.end(); it2++ )
			{
				(*it2)->setRenderRemoveStage(TileEntity::e_RenderRemoveStageFlaggedAtChunk);
			}
		}
	}
	LeaveCriticalSection(globalRenderableTileEntities_cs);
#else
	// Find the removed ones:

	// 4J - original code for this section:
	/*
		Set<TileEntity> newTileEntities = new HashSet<TileEntity>();
		newTileEntities.addAll(renderableTileEntities);
		newTileEntities.removeAll(oldTileEntities);
		globalRenderableTileEntities.addAll(newTileEntities);

		oldTileEntities.removeAll(renderableTileEntities);
		globalRenderableTileEntities.removeAll(oldTileEntities);
		*/
        

    std::unordered_set<std::shared_ptr<TileEntity> > newTileEntities(renderableTileEntities.begin(),renderableTileEntities.end());
    
	AUTO_VAR(endIt, oldTileEntities.end());
	for( std::unordered_set<std::shared_ptr<TileEntity> >::iterator it = oldTileEntities.begin(); it != endIt; it++ )
	{
		newTileEntities.erase(*it);
	}

	// 4J - newTileEntities is now renderableTileEntities with any old ones from oldTileEntitesRemoved (so just new things added)

	EnterCriticalSection(globalRenderableTileEntities_cs);
	endIt = newTileEntities.end();
	for( std::unordered_set<std::shared_ptr<TileEntity> >::iterator it = newTileEntities.begin(); it != endIt; it++ )
	{
		globalRenderableTileEntities.push_back(*it);
	}

	// 4J - All these new things added to globalRenderableTileEntities

	AUTO_VAR(endItRTE, renderableTileEntities.end());
	for( std::vector<std::shared_ptr<TileEntity> >::iterator it = renderableTileEntities.begin(); it != endItRTE; it++ )
	{
		oldTileEntities.erase(*it);
	}
	// 4J - oldTileEntities is now the removed items
	std::vector<std::shared_ptr<TileEntity> >::iterator it = globalRenderableTileEntities->begin();
	while(  it != globalRenderableTileEntities->end() )
	{
		if( oldTileEntities.find(*it) != oldTileEntities.end() )
		{
			it = globalRenderableTileEntities->erase(it);
		}
		else
		{
			++it;
		}
	}

	LeaveCriticalSection(globalRenderableTileEntities_cs);
#endif

	// 4J - These removed items are now also removed from globalRenderableTileEntities

	if( LevelChunk::touchedSky )
	{
		levelRenderer->clearGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
	}
	else
	{
		levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
	}
	levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_COMPILED);
	return;

}
#endif // _PS3_


float Chunk::distanceToSqr(std::shared_ptr<Entity> player) const
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym);
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr(std::shared_ptr<Entity> player)
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym) * 2;
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

void Chunk::reset()
{
	if( assigned )
	{
		EnterCriticalSection(&levelRenderer->m_csDirtyChunks);
		unsigned char refCount = levelRenderer->decGlobalChunkRefCount(x, y, z, level);
		assigned = false;
//		printf("\t\t [dec] refcount %d at %d, %d, %d\n",refCount,x,y,z);
		if( refCount == 0 )
		{
			int lists = levelRenderer->getGlobalIndexForChunk(x, y, z, level) * 2;
			if(lists >= 0)
			{
				lists += levelRenderer->chunkLists;
				for (int i = 0; i < 2; i++)
				{
					// 4J - added - clear any renderer data associated with this unused list
					RenderManager.CBuffClear(lists + i);
				}
				levelRenderer->setGlobalChunkFlags(x, y, z, level, 0);
			}
		}
		LeaveCriticalSection(&levelRenderer->m_csDirtyChunks);
	}

	clipChunk->visible = false;
}

void Chunk::_delete()
{
	reset();
	level = NULL;
}

int Chunk::getList(int layer)
{
	if (!clipChunk->visible) return -1;

	int lists = levelRenderer->getGlobalIndexForChunk(x, y, z,level) * 2;
	lists += levelRenderer->chunkLists;

	bool empty =  levelRenderer->getGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
	if (!empty) return lists + layer;
	return -1;
}

void Chunk::cull(Culler *culler)
{
	clipChunk->visible = culler->isVisible(bb);
}

void Chunk::renderBB()
{
//	glCallList(lists + 2);	// 4J - removed - TODO put back in
}

bool Chunk::isEmpty()
{
	if (!levelRenderer->getGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_COMPILED)) return false;
	return levelRenderer->getGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTYBOTH);
}

void Chunk::setDirty()
{
	// 4J - not used, but if this starts being used again then we'll need to investigate how best to handle it.
	__debugbreak();
	levelRenderer->setGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_DIRTY);
}

void Chunk::clearDirty()
{
	levelRenderer->clearGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_DIRTY);
#ifdef _CRITICAL_CHUNKS
	levelRenderer->clearGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_CRITICAL);
#endif
}

Chunk::~Chunk()
{
	delete bb;
}

bool Chunk::emptyFlagSet(int layer)
{
	return levelRenderer->getGlobalChunkFlag(x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
}
