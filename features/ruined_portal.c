#include "ruined_portal.h"
#include "../loot/loot_tables.h"
#include "../loot/items.h"

#include <string.h>

#define RP_COL_H 152
#define RP_DENS_CELLS 20

typedef struct { int8_t y, z; } RPCell;

typedef struct
{
    const char *name;
    int8_t sx, sy, sz;
    int8_t chestX, chestY, chestZ;
    int8_t planeX;
    int8_t fy0, fy1, fz0, fz1;
    uint8_t nObs;
    uint8_t nGap;
    RPCell obs[RP_MAX_FRAME_CELLS];
    RPCell gap[RP_MAX_FRAME_CELLS];
} RPTemplate;

static const RPTemplate g_rp_templates[13] =
{
    { "portal_1", 6,10,6, 2,2,0, 3, 2,6,1,4, 8, 2,
      { {2,2}, {2,3}, {3,1}, {3,4}, {4,1}, {5,1}, {6,2}, {6,3} },
      { {4,4}, {5,4} } },
    { "portal_2", 9,12,9, 8,2,6, 5, 4,8,2,5, 6, 4,
      { {5,2}, {6,2}, {7,2}, {7,5}, {8,3}, {8,4} },
      { {4,3}, {4,4}, {5,5}, {6,5} } },
    { "portal_3", 8,9,9, 3,3,6, 4, 3,7,2,5, 6, 4,
      { {3,3}, {3,4}, {4,5}, {5,5}, {6,5}, {7,4} },
      { {4,2}, {5,2}, {6,2}, {7,3} } },
    { "portal_4", 8,9,9, 3,3,2, 4, 3,6,2,5, 6, 2,
      { {3,3}, {3,4}, {4,2}, {4,5}, {5,2}, {5,5} },
      { {6,3}, {6,4} } },
    { "portal_5", 10,10,7, 4,3,2, 2, 3,8,1,4, 6, 6,
      { {3,2}, {3,3}, {4,4}, {5,4}, {6,4}, {7,4} },
      { {4,1}, {5,1}, {6,1}, {7,1}, {8,2}, {8,3} } },
    { "portal_6", 5,7,7, 1,1,4, 2, 1,5,0,4, 11, 1,
      { {1,1}, {1,2}, {1,3}, {2,0}, {2,4}, {3,0}, {3,4}, {4,0}, {4,4}, {5,1}, {5,3} },
      { {5,2} } },
    { "portal_7", 9,7,9, 0,1,2, 3, 0,4,2,5, 9, 1,
      { {0,3}, {0,4}, {1,2}, {1,5}, {2,2}, {2,5}, {3,2}, {4,3}, {4,4} },
      { {3,5} } },
    { "portal_8", 14,9,9, 4,4,2, 5, 3,8,2,6, 11, 3,
      { {3,3}, {3,4}, {3,5}, {4,2}, {4,6}, {5,2}, {5,6}, {6,2}, {6,6}, {7,2}, {7,6} },
      { {8,3}, {8,4}, {8,5} } },
    { "portal_9", 10,8,9, 4,1,0, 4, 1,5,3,6, 8, 2,
      { {1,4}, {1,5}, {2,3}, {2,6}, {3,6}, {4,6}, {5,4}, {5,5} },
      { {3,3}, {4,3} } },
    { "portal_10", 12,8,10, 2,1,7, 3, 1,2,3,6, 2, 2,
      { {1,4}, {1,5} },
      { {2,4}, {2,5} } },
    { "giant_portal_1", 11,17,16, 4,3,3, 5, 3,12,4,11, 20, 8,
      { {3,5}, {3,6}, {3,7}, {3,8}, {3,9}, {3,10}, {4,4}, {4,11}, {5,11}, {6,11},
        {7,11}, {8,4}, {8,11}, {9,4}, {9,11}, {10,4}, {11,4}, {12,5}, {12,6}, {12,7} },
      { {5,4}, {6,4}, {7,4}, {10,11}, {11,11}, {12,8}, {12,9}, {12,10} } },
    { "giant_portal_2", 11,16,16, 9,1,9, 5, 3,12,4,11, 19, 9,
      { {3,5}, {3,6}, {3,7}, {3,8}, {3,9}, {3,10}, {4,4}, {4,11}, {5,11}, {6,11},
        {7,11}, {8,11}, {9,11}, {10,4}, {10,11}, {11,4}, {11,11}, {12,7}, {12,8} },
      { {5,4}, {6,4}, {7,4}, {8,4}, {9,4}, {12,5}, {12,6}, {12,9}, {12,10} } },
    { "giant_portal_3", 16,16,16, 9,2,3, 5, 3,12,4,11, 14, 14,
      { {3,5}, {3,6}, {3,7}, {3,8}, {3,9}, {3,10}, {4,4}, {4,11}, {5,11}, {6,11},
        {7,11}, {8,11}, {9,11}, {12,9} },
      { {5,4}, {6,4}, {7,4}, {8,4}, {9,4}, {10,4}, {10,11}, {11,4}, {11,11},
        {12,5}, {12,6}, {12,7}, {12,8}, {12,10} } },
};


static int rpPortalType(int mc, int biomeID)
{
    switch (getCategory(mc, biomeID))
    {
    case desert:        return desert;
    case jungle:        return jungle;
    case swamp:         return swamp;
    case ocean:         return ocean;
    case nether_wastes: return nether_wastes;
    default: break;
    }
    switch (biomeID)
    {
    case mountains:                         // 3
    case snowy_mountains:                   // 13
    case mountain_edge:                     // 20
    case stone_shore:                       // 25
    case wooded_mountains:                  // 34
    case savanna_plateau:                   // 36
    case wooded_badlands_plateau:           // 38
    case badlands_plateau:                  // 39
    case gravelly_mountains:                // 131
    case taiga_mountains:                   // 133
    case snowy_taiga_mountains:             // 158
    case modified_gravelly_mountains:       // 162
    case shattered_savanna:                 // 163
    case shattered_savanna_plateau:         // 164
    case eroded_badlands:                   // 165
    case modified_wooded_badlands_plateau:  // 166
    case modified_badlands_plateau:         // 167
        return mountains;
    }
    return plains;
}

int isCryingObsidian(int x, int y, int z)
{
    uint64_t l = (uint64_t)(int64_t)(int32_t)((uint32_t)x * 3129871u)
               ^ (uint64_t)((int64_t)z * 116129781LL)
               ^ (uint64_t)(int64_t)y;
    l = l * l * 42317861ULL + l * 11ULL;

    uint64_t rng;
    setSeed(&rng, (uint64_t)((int64_t)l >> 16));
    return nextFloat(&rng) < 0.15f;
}

static Pos3 rpTransform(int x, int y, int z, int mirror, int rotation, int px, int pz)
{
    Pos3 r;
    if (mirror)
        x = -x;
    switch (rotation)
    {
    case 1:  r = (Pos3) {px + pz - z, y, pz - px + x}; break;
    case 2:  r = (Pos3) {px + px - x, y, pz + pz - z}; break;
    case 3:  r = (Pos3) {px - pz + z, y, px + pz - x}; break;
    default: r = (Pos3) {x, y, z}; break;
    }
    return r;
}

static void rpColumn(const Generator *g, const SurfaceNoise *sn, int x, int z,
        double *out)
{
    double c[2][2][RP_DENS_CELLS];
    int px = x >> 2, pz = z >> 2;
    double fx = (x & 3) / 4.0, fz = (z & 3) / 4.0;
    int y;

    surfaceCornerDens(g, sn, px+0, pz+0, c[0][0]);
    surfaceCornerDens(g, sn, px+1, pz+0, c[1][0]);
    surfaceCornerDens(g, sn, px+0, pz+1, c[0][1]);
    surfaceCornerDens(g, sn, px+1, pz+1, c[1][1]);

    for (y = 0; y < RP_COL_H; y++)
    {
        int py = y >> 3;
        double fy = (y & 7) / 8.0;
        double l00 = lerp(fy, c[0][0][py], c[0][0][py+1]);
        double l10 = lerp(fy, c[1][0][py], c[1][0][py+1]);
        double l01 = lerp(fy, c[0][1][py], c[0][1][py+1]);
        double l11 = lerp(fy, c[1][1][py], c[1][1][py+1]);
        out[y] = lerp(fz, lerp(fx, l00, l10), lerp(fx, l01, l11));
    }
}

static inline int rpOpaque(double dens, int y, int oceanFloor)
{
    if (dens > 0)
        return 1;
    if (oceanFloor)
        return 0;
    return y < 63;
}

static int rpBaseHeight(const double *col, int oceanFloor)
{
    int y;
    for (y = RP_COL_H - 1; y >= 0; y--)
        if (rpOpaque(col[y], y, oceanFloor))
            return y + 1;
    return 0;
}

static int rpFindSuitableY(uint64_t *rng, int placement, int airPocket,
        int baseY, int ySpan, const double corner[4][RP_COL_H], int oceanFloor)
{
    int k, m;

    if (placement == RP_IN_NETHER)
    {
        if (airPocket)
            k = nextInt(rng, 69) + 32;
        else if (nextFloat(rng) < 0.5f)
            k = nextInt(rng, 3) + 27;
        else
            k = nextInt(rng, 72) + 29;
    }
    else if (placement == RP_IN_MOUNTAIN)
    {
        int l = baseY - ySpan;
        k = (70 < l) ? nextInt(rng, l - 70 + 1) + 70 : l;
    }
    else if (placement == RP_UNDERGROUND)
    {
        int l = baseY - ySpan;
        k = (15 < l) ? nextInt(rng, l - 15 + 1) + 15 : l;
    }
    else if (placement == RP_PARTLY_BURIED)
    {
        k = baseY - ySpan + nextInt(rng, 7) + 2;
    }
    else
    {
        k = baseY;
    }

    for (m = k; m > 15; m--)
    {
        int n = 0, c;
        if (m >= RP_COL_H)
            continue; // nothing but air up here
        for (c = 0; c < 4; c++)
        {
            if (rpOpaque(corner[c][m], m, oceanFloor) && ++n == 3)
                return m;
        }
    }
    return m;
}


static int rpResolve(RuinedPortal *out, const Generator *g, const SurfaceNoise *sn,
        LootTableContext *loot, int x, int z, int earlyOut)
{
    StructureSaltConfig ssconf;
    const RPTemplate *t;
    uint64_t rng, lootRng, pop;
    int cx = x >> 4, cz = z >> 4;
    int ox = cx << 4, oz = cz << 4;
    int biomeID, type, placement, airPocket = 0, giant, idx, rotation, mirror;
    int pivotX, pivotZ, oceanFloor;
    int bx0, bx1, bz0, bz1, cex, cez, i;
    double corner[4][RP_COL_H], center[RP_COL_H];
    Pos3 p;

    memset(out, 0, sizeof(*out));

    if (g->mc != MC_1_16_1 && g->mc != MC_1_16_5)
        return RP_ERR_VERSION;
    if (g->dim != DIM_OVERWORLD)
        return RP_ERR_VERSION;
    if (loot == NULL)
        return RP_ERR_LOOT;

    biomeID = getBiomeAt(g, 4, cx*4 + 2, 0, cz*4 + 2);
    if (biomeID < 0)
        return RP_ERR_NO_STRUCTURE;
    if (!getStructureSaltConfig(Ruined_Portal, g->mc, biomeID, &ssconf))
        return RP_ERR_NO_STRUCTURE;

    type = rpPortalType(g->mc, biomeID);
    rng = chunkGenerateRnd(g->seed, cx, cz);
    switch (type)
    {
    case desert:
        placement = RP_PARTLY_BURIED;
        break;
    case jungle:
        placement = RP_ON_LAND_SURFACE;
        airPocket = nextFloat(&rng) < 0.5f;
        break;
    case swamp:
    case ocean:
        placement = RP_ON_OCEAN_FLOOR;
        break;
    case nether_wastes:
        placement = RP_IN_NETHER;
        airPocket = nextFloat(&rng) < 0.5f;
        break;
    case mountains:
    case plains:
    default:
    {
        int deep = nextFloat(&rng) < 0.5f;
        if (type == mountains)
            placement = deep ? RP_IN_MOUNTAIN : RP_ON_LAND_SURFACE;
        else
            placement = deep ? RP_UNDERGROUND : RP_ON_LAND_SURFACE;
        airPocket = deep ? 1 : (nextFloat(&rng) < 0.5f);
        break;
    }
    }
    if (placement == RP_IN_NETHER)
        return RP_ERR_VERSION; // needs nether terrain, not supported

    giant = nextFloat(&rng) < 0.05f;
    idx = giant ? 10 + nextInt(&rng, 3) : nextInt(&rng, 10);
    rotation = nextInt(&rng, 4);
    mirror = nextFloat(&rng) >= 0.5f;

    t = &g_rp_templates[idx];
    pivotX = t->sx / 2;
    pivotZ = t->sz / 2;
    oceanFloor = (placement == RP_ON_OCEAN_FLOOR);

    out->templateIdx    = idx;
    out->templateName   = t->name;
    out->rotation       = rotation;
    out->mirror         = mirror;
    out->placement      = placement;
    out->portalType     = type;
    out->airPocket      = airPocket;
    out->frameW         = t->fz1 - t->fz0 + 1;
    out->frameH         = t->fy1 - t->fy0 + 1;
    out->frameUsable    = out->frameW >= 4 && out->frameH >= 5;
    out->obsidianNeeded = t->nGap;

    pop = getPopulationSeed(g->mc, g->seed, ox, oz);
    setSeed(&lootRng, pop + ssconf.decoratorIndex + 10000ULL * ssconf.generationStep);
    out->lootSeed = nextLong(&lootRng);

    set_loot_seed(loot, out->lootSeed);
    generate_loot(loot);
    for (i = 0; i < loot->generated_item_count; i++)
    {
        ItemStack *is = &loot->generated_items[i];
        switch (get_global_item_id(loot, is->item))
        {
        case ITEM_OBSIDIAN:        out->obsidianInChest += is->count; break;
        case ITEM_FLINT_AND_STEEL: out->flintAndSteel   += is->count; break;
        case ITEM_FIRE_CHARGE:     out->fireCharge      += is->count; break;
        case ITEM_FLINT:           out->flint           += is->count; break;
        case ITEM_IRON_NUGGET:     out->ironNuggets     += is->count; break;
        default: break;
        }
    }
    out->hasIgniter = out->flintAndSteel > 0 || out->fireCharge > 0 ||
                      (out->flint > 0 && out->ironNuggets >= 9);

    out->pos.x = ox;
    out->pos.z = oz;
    out->pos.y = RP_Y_UNRESOLVED;

    if (earlyOut && (!out->frameUsable || !out->hasIgniter ||
                     out->obsidianInChest < out->obsidianNeeded))
        return RP_OK; // already ruled out, skip the terrain work

    bx0 = bz0 = 0x7fffffff;
    bx1 = bz1 = -0x7fffffff;
    for (i = 0; i < 4; i++)
    {
        p = rpTransform((i & 1) ? t->sx - 1 : 0, 0, (i & 2) ? t->sz - 1 : 0,
                mirror, rotation, pivotX, pivotZ);
        if (p.x < bx0) bx0 = p.x;
        if (p.x > bx1) bx1 = p.x;
        if (p.z < bz0) bz0 = p.z;
        if (p.z > bz1) bz1 = p.z;
    }
    bx0 += ox; bx1 += ox;
    bz0 += oz; bz1 += oz;

    cex = bx0 + (bx1 - bx0 + 1) / 2;
    cez = bz0 + (bz1 - bz0 + 1) / 2;

    rpColumn(g, sn, cex, cez, center);
    rpColumn(g, sn, bx0, bz0, corner[0]);
    rpColumn(g, sn, bx1, bz0, corner[1]);
    rpColumn(g, sn, bx0, bz1, corner[2]);
    rpColumn(g, sn, bx1, bz1, corner[3]);

    out->bx0 = bx0; out->bx1 = bx1;
    out->bz0 = bz0; out->bz1 = bz1;
    out->pos.y = rpFindSuitableY(&rng, placement, airPocket,
            rpBaseHeight(center, oceanFloor) - 1, t->sy, corner, oceanFloor);

    p = rpTransform(t->chestX, t->chestY, t->chestZ, mirror, rotation, pivotX, pivotZ);
    out->chest.x = p.x + ox;
    out->chest.y = p.y + out->pos.y;
    out->chest.z = p.z + oz;

    out->frameCount = t->nObs;
    for (i = 0; i < t->nObs; i++)
    {
        p = rpTransform(t->planeX, t->obs[i].y, t->obs[i].z,
                mirror, rotation, pivotX, pivotZ);
        p.x += ox;
        p.y += out->pos.y;
        p.z += oz;
        out->framePos[i] = p;
        out->frameCrying[i] = isCryingObsidian(p.x, p.y, p.z);
        out->cryingCount += out->frameCrying[i];
    }

    out->completable = out->frameUsable && out->cryingCount == 0 &&
                       out->obsidianInChest >= out->obsidianNeeded &&
                       out->hasIgniter;
    return RP_OK;
}

int getRuinedPortalTemplate(RuinedPortalTemplate *out, int idx)
{
    const RPTemplate *t;
    int i;

    if (idx < 0 || idx >= 13)
        return 0;
    t = &g_rp_templates[idx];

    memset(out, 0, sizeof(*out));
    out->name   = t->name;
    out->planeX = t->planeX;
    out->fy0    = t->fy0;
    out->fy1    = t->fy1;
    out->fz0    = t->fz0;
    out->fz1    = t->fz1;
    out->nObs   = t->nObs;
    out->nGap   = t->nGap;
    for (i = 0; i < t->nObs; i++)
    {
        out->obs[i][0] = t->obs[i].y;
        out->obs[i][1] = t->obs[i].z;
    }
    for (i = 0; i < t->nGap; i++)
    {
        out->gap[i][0] = t->gap[i].y;
        out->gap[i][1] = t->gap[i].z;
    }
    return 1;
}

int getRuinedPortal(RuinedPortal *out, const Generator *g, const SurfaceNoise *sn,
        LootTableContext *loot, int x, int z)
{
    return rpResolve(out, g, sn, loot, x, z, 0);
}

int isRuinedPortalCompletable(const Generator *g, const SurfaceNoise *sn,
        LootTableContext *loot, int x, int z)
{
    RuinedPortal rp;
    if (rpResolve(&rp, g, sn, loot, x, z, 1) != RP_OK)
        return 0;
    return rp.completable;
}
