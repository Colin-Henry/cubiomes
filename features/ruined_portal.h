#ifndef RUINED_PORTAL_H_
#define RUINED_PORTAL_H_

#include "../finders.h"
#include "../loot/loot_table_context.h"

#ifdef __cplusplus
extern "C" {
#endif

// RuinedPortalPiece.VerticalPlacement
enum RuinedPortalPlacement
{
    RP_ON_LAND_SURFACE,
    RP_PARTLY_BURIED,
    RP_ON_OCEAN_FLOOR,
    RP_IN_MOUNTAIN,
    RP_UNDERGROUND,
    RP_IN_NETHER,
};

// error codes returned by getRuinedPortal()
enum
{
    RP_OK               =  0,
    RP_ERR_VERSION      = -1,   // only 1.16.x overworld is supported
    RP_ERR_NO_STRUCTURE = -2,   // no ruined portal generates at this position
    RP_ERR_LOOT         = -3,   // loot table context missing / wrong table
};

enum { RP_MAX_FRAME_CELLS = 30 };

// pos.y when the placement height was not computed, see
// isRuinedPortalCompletable()
enum { RP_Y_UNRESOLVED = -0x40000000 };

STRUCT(RuinedPortal)
{
    int templateIdx;            // 0..9 = portal_1..10, 10..12 = giant_portal_1..3
    const char *templateName;
    int rotation;               // 0:none, 1:cw90, 2:cw180, 3:ccw90
    int mirror;                 // 0:none, 1:front_back
    int placement;              // enum RuinedPortalPlacement
    int portalType;             // biome id the portal type was chosen from
    int airPocket;

    Pos3 pos;                   // template origin: chunk corner in xz, placement y
    int bx0, bz0, bx1, bz1;     // world-space bounding box in xz
    int frameW, frameH;         // portal frame size including corners

    Pos3 chest;                 // world position of the chest
    uint64_t lootSeed;

    // completion requirements
    int obsidianNeeded;         // non-corner frame blocks missing from the template
    int obsidianInChest;

    // the non-corner frame blocks the template does provide, in world space.
    // frameCrying[i] is 1 when that block generated as crying obsidian, which
    // cannot be mined without a diamond pickaxe and so blocks completion.
    int frameCount;
    Pos3 framePos[RP_MAX_FRAME_CELLS];
    uint8_t frameCrying[RP_MAX_FRAME_CELLS];
    int cryingCount;

    int flintAndSteel;          // item counts in the chest
    int fireCharge;
    int flint;
    int ironNuggets;

    int hasIgniter;             // flint&steel, fire charge, or flint + 9 iron nuggets
    int frameUsable;            // frame is at least 4 wide and 5 tall
    int completable;
};

/* Resolves the ruined portal whose structure position is (x, z)
 * Pass the position returned by getStructurePos 
 * Fills 'out' with the template, placement height, chest loot and the frame data
 * Returns RP_OK, or a negative RP_ERR_* code.
 */
int getRuinedPortal(RuinedPortal *out, const Generator *g, const SurfaceNoise *sn, LootTableContext *loot, int x, int z);

/**
 * Determines if a ruined portal is completable
 * Completable means no crying obi (corners excluded) and non-obsidian
 * blocks can be filled with obsidian found in the chest, as well as a way
 * to light the portal
 * @param g the generator (make sure to use applySeed)
 * @param sn surface noise
 * @param loot the loot table for RPs
 * @param x x-coordinate of the structure
 * @param z z-coordinate of the structure
*/
int isRuinedPortalCompletable(const Generator *g, const SurfaceNoise *sn, LootTableContext *loot, int x, int z);

// determines if the block is crying obsidian (its only based on world coordinates)
int isCryingObsidian(int x, int y, int z);

STRUCT(RuinedPortalTemplate)
{
    const char *name;
    int planeX;
    int fy0, fy1, fz0, fz1;     // frame rectangle, corners included
    int nObs, nGap;
    int obs[RP_MAX_FRAME_CELLS][2];     // {y, z}
    int gap[RP_MAX_FRAME_CELLS][2];
};

// fills "out" with template "idx" (0..12), returns 0 when idx is out of range
int getRuinedPortalTemplate(RuinedPortalTemplate *out, int idx);

#ifdef __cplusplus
}
#endif

#endif /* RUINED_PORTAL_H_ */
