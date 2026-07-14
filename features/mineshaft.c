#include "mineshaft.h"

#include <stdio.h>
#include <string.h>

#include "piece.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

STRUCT(MineshaftPieceEnv) {
    int mc;
    Piece *list;
    int *n;
    uint64_t *rng;
    int nmax;
};

static inline Piece* hasCollision(MineshaftPieceEnv *env, Pos3 b0, Pos3 b1) {
    int i, n = *env->n;
    for (i = 0; i < n; i++) {
        Piece *q = env->list + i;
        if (hasIntersection(q->bb0, q->bb1, b0, b1)) {
            return q;
        }
    }
    return NULL;
}

static void extendMineshaftPiece(MineshaftPieceEnv *env, Piece *piece);

Pos3 entrance1[64], entrance2[64];
int entranceCount;

static void extendMineshaft(MineshaftPieceEnv *env, int x, int y, int z, int facing, int depth) {
    if (depth > 8) {
        return;
    }

    if (IABS(x - env->list->bb0.x) > 80 || IABS(z - env->list->bb0.z) > 80) {
        return;
    }

    depth += 1;

    int randomSelection = nextInt(env->rng, 100);
    if (randomSelection >= 80) {
        int y1 = 2 + 4 * (nextInt(env->rng, 4) == 0);
        Pos3 bb0, bb1;
        switch (facing) {
        case 0: bb0 = (Pos3) {x + -1, y, z + -4}; bb1 = (Pos3) {x + 3, y + y1, z + 0}; break;
        case 1: bb0 = (Pos3) {x + 0, y, z + -1}; bb1 = (Pos3) {x + 4, y + y1, z + 3}; break;
        case 2: bb0 = (Pos3) {x + -1, y, z + 0}; bb1 = (Pos3) {x + 3, y + y1, z + 4}; break;
        case 3: bb0 = (Pos3) {x + -4, y, z + -1}; bb1 = (Pos3) {x + 0, y + y1, z + 3}; break;
        default: UNREACHABLE();
        }
        if (!hasCollision(env, bb0, bb1)) {
            Piece *p = env->list + (*env->n)++;
            p->name = "MSCrossing";
            p->pos = bb0;
            p->bb0 = bb0;
            p->bb1 = bb1;
            p->rot = facing;
            p->depth = depth;
            p->type = MS_CROSSING;
            p->next = NULL;
            p->additionalData = 0;
            extendMineshaftPiece(env, p);
            return;
        }
    } else if (randomSelection >= 70) {
        Pos3 bb0, bb1;
        switch (facing) {
        case 0: bb0 = (Pos3) {x + 0, y + -5, z + -8}; bb1 = (Pos3) {x + 2, y + 2, z + 0}; break;
        case 1: bb0 = (Pos3) {x + 0, y + -5, z + 0}; bb1 = (Pos3) {x + 8, y + 2, z + 2}; break;
        case 2: bb0 = (Pos3) {x + 0, y + -5, z + 0}; bb1 = (Pos3) {x + 2, y + 2, z + 8}; break;
        case 3: bb0 = (Pos3) {x + -8, y + -5, z + 0}; bb1 = (Pos3) {x + 0, y + 2, z + 2}; break;
        default: UNREACHABLE();
        }
        if (!hasCollision(env, bb0, bb1)) {
            Piece *p = env->list + (*env->n)++;
            p->name = "MSStairs";
            p->pos = bb0;
            p->bb0 = bb0;
            p->bb1 = bb1;
            p->rot = facing;
            p->depth = depth;
            p->type = MS_STAIRS;
            p->next = NULL;
            p->additionalData = 0;
            extendMineshaftPiece(env, p);
            return;
        }
    } else {
        for (int corridorLength = nextInt(env->rng, 3) + 2; corridorLength > 0; corridorLength--) {
            int blockLength = corridorLength * 5;

            Pos3 bb0, bb1;
            switch (facing) {
            case 0: bb0 = (Pos3) {x + 0, y, z + -(blockLength - 1)}; bb1 = (Pos3) {x + 2, y + 2, z + 0}; break;
            case 1: bb0 = (Pos3) {x + 0, y, z + 0}; bb1 = (Pos3) {x + blockLength - 1, y + 2, z + 2}; break;
            case 2: bb0 = (Pos3) {x + 0, y, z + 0}; bb1 = (Pos3) {x + 2, y + 2, z + blockLength - 1}; break;
            case 3: bb0 = (Pos3) {x + -(blockLength - 1), y, z + 0}; bb1 = (Pos3) {x + 0, y + 2, z + 2}; break;
            default: UNREACHABLE();
            }
            if (!hasCollision(env, bb0, bb1)) {
                Piece *p = env->list + (*env->n)++;
                p->name = "MSCorridor";
                p->pos = bb0;
                p->bb0 = bb0;
                p->bb1 = bb1;
                p->rot = facing;
                p->depth = depth;
                p->type = MS_CORRIDOR;
                p->next = NULL;
                p->additionalData = 0;

                int hasRails = nextInt(env->rng, 3) == 0;
                p->additionalData |= hasRails << 0;
                p->additionalData |= (!hasRails && nextInt(env->rng, 23) == 0) << 1; // spiderCorridor

                extendMineshaftPiece(env, p);
                return;
            }
        } // TODO check corridorLength > 0 check in older versions
    }
}

static void extendMineshaftPiece(MineshaftPieceEnv *env, Piece *piece) {
    if (*env->n >= env->nmax) {
        return;
    }
    switch (piece->type) {
    case MS_CORRIDOR: {
        int endSelection = nextInt(env->rng, 4);
        int rot = piece->rot;
        switch (rot) {
        case 0:
            if (endSelection <= 1) {
                extendMineshaft(env, piece->bb0.x, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z - 1, 0, piece->depth);
            } else if (endSelection == 2) {
                extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z, 3, piece->depth);
            } else {
                extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z, 1, piece->depth);
            }
            break;
        case 1:
            if (endSelection <= 1) {
                extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z, 1, piece->depth);
            } else if (endSelection == 2) {
                extendMineshaft(env, piece->bb1.x - 3, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z - 1, 0, piece->depth);
            } else {
                extendMineshaft(env, piece->bb1.x - 3, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb1.z + 1, 2, piece->depth);
            }
            break;
        case 2:
            if (endSelection <= 1) {
                extendMineshaft(env, piece->bb0.x, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb1.z + 1, 2, piece->depth);
            } else if (endSelection == 2) {
                extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb1.z - 3, 3, piece->depth);
            } else {
                extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb1.z - 3, 1, piece->depth);
            }
            break;
        case 3:
            if (endSelection <= 1) {
                extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z, 3, piece->depth);
            } else if (endSelection == 2) {
                extendMineshaft(env, piece->bb0.x, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb0.z - 1, 0, piece->depth);
            } else {
                extendMineshaft(env, piece->bb0.x, piece->bb0.y - 1 + nextInt(env->rng, 3), piece->bb1.z + 1, 2, piece->depth);
            }
            break;
        default: UNREACHABLE();
        }

        if (piece->depth >= 8) {
            break;
        }
        if (rot != 0 && rot != 2) {
            for (int x = piece->bb0.x + 3; x + 3 <= piece->bb1.x; x += 5) {
                int selection = nextInt(env->rng, 5);
                if (selection == 0) {
                    extendMineshaft(env, x, piece->bb0.y, piece->bb0.z - 1, 0, piece->depth + 1);
                } else if (selection == 1) {
                    extendMineshaft(env, x, piece->bb0.y, piece->bb1.z + 1, 2, piece->depth + 1);
                }
            }
        } else {
            for (int z = piece->bb0.z + 3; z + 3 <= piece->bb1.z; z += 5) {
                int selection = nextInt(env->rng, 5);
                if (selection == 0) {
                    extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y, z, 3, piece->depth + 1);
                } else if (selection == 1) {
                    extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y, z, 1, piece->depth + 1);
                }
            }
        }
        break;
    }
    case MS_CROSSING: {
        switch (piece->rot) {
        case 0:
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb0.z - 1, 0, piece->depth);
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y, piece->bb0.z + 1, 3, piece->depth);
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y, piece->bb0.z + 1, 1, piece->depth);
            break;
        case 1:
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb0.z - 1, 0, piece->depth);
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb1.z + 1, 2, piece->depth);
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y, piece->bb0.z + 1, 1, piece->depth);
            break;
        case 2:
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb1.z + 1, 2, piece->depth);
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y, piece->bb0.z + 1, 3, piece->depth);
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y, piece->bb0.z + 1, 1, piece->depth);
            break;
        case 3:
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb0.z - 1, 0, piece->depth);
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y, piece->bb1.z + 1, 2, piece->depth);
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y, piece->bb0.z + 1, 3, piece->depth);
            break;
        default: UNREACHABLE();
        }

        int isTwoFloored = piece->bb1.y - piece->bb0.y + 1 > 3;
        if (!isTwoFloored) {
            break;
        }
        if (next(env->rng, 1)) {
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y + 3 + 1, piece->bb0.z - 1, 0, piece->depth);
        }
        if (next(env->rng, 1)) {
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y + 3 + 1, piece->bb0.z + 1, 3, piece->depth);
        }
        if (next(env->rng, 1)) {
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y + 3 + 1, piece->bb0.z + 1, 1, piece->depth);
        }
        if (next(env->rng, 1)) {
            extendMineshaft(env, piece->bb0.x + 1, piece->bb0.y + 3 + 1, piece->bb1.z + 1, 2, piece->depth);
        }
        break;
    }
    case MS_ROOM: {
        int heightSpace = (piece->bb1.y - piece->bb0.y + 1) - 3 - 1;
        if (heightSpace <= 0) {
            heightSpace = 1;
        }
        Pos3 rb0 = piece->bb0, rb1 = piece->bb1;
        int before;
        Piece *c;

        int xSpan = piece->bb1.x - piece->bb0.x + 1;
        for (int pos = 0; pos < xSpan; pos += 4) {
            pos += nextInt(env->rng, xSpan);
            if (pos + 3 > xSpan) {
                break;
            }
            before = *env->n;
            extendMineshaft(env, piece->bb0.x + pos, piece->bb0.y + nextInt(env->rng, heightSpace) + 1, piece->bb0.z - 1, 0, piece->depth);
            if (*env->n > before && entranceCount < 64) {
                c = env->list + before;
                entrance1[entranceCount] = (Pos3) {c->bb0.x, c->bb0.y, rb0.z};
                entrance2[entranceCount] = (Pos3) {c->bb1.x, c->bb1.y, rb0.z + 1};
                entranceCount++;
            }
        }

        for (int pos = 0; pos < xSpan; pos += 4) {
            pos += nextInt(env->rng, xSpan);
            if (pos + 3 > xSpan) {
                break;
            }
            before = *env->n;
            extendMineshaft(env, piece->bb0.x + pos, piece->bb0.y + nextInt(env->rng, heightSpace) + 1, piece->bb1.z + 1, 2, piece->depth);
            if (*env->n > before && entranceCount < 64) {
                c = env->list + before;
                entrance1[entranceCount] = (Pos3) {c->bb0.x, c->bb0.y, rb1.z - 1};
                entrance2[entranceCount] = (Pos3) {c->bb1.x, c->bb1.y, rb1.z};
                entranceCount++;
            }
        }

        int zSpan = piece->bb1.z - piece->bb0.z + 1;
        for (int pos = 0; pos < zSpan; pos += 4) {
            pos += nextInt(env->rng, zSpan);
            if (pos + 3 > zSpan) {
                break;
            }
            before = *env->n;
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y + nextInt(env->rng, heightSpace) + 1, piece->bb0.z + pos, 3, piece->depth);
            if (*env->n > before && entranceCount < 64) {
                c = env->list + before;
                entrance1[entranceCount] = (Pos3) {rb0.x, c->bb0.y, c->bb0.z};
                entrance2[entranceCount] = (Pos3) {rb0.x + 1, c->bb1.y, c->bb1.z};
                entranceCount++;
            }
        }

        for (int pos = 0; pos < zSpan; pos += 4) {
            pos += nextInt(env->rng, zSpan);
            if (pos + 3 > zSpan) {
                break;
            }
            before = *env->n;
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y + nextInt(env->rng, heightSpace) + 1, piece->bb0.z + pos, 1, piece->depth);
            if (*env->n > before && entranceCount < 64) {
                c = env->list + before;
                entrance1[entranceCount] = (Pos3) {rb1.x - 1, c->bb0.y, c->bb0.z};
                entrance2[entranceCount] = (Pos3) {rb1.x, c->bb1.y, c->bb1.z};
                entranceCount++;
            }
        }
        break;
    }
    case MS_STAIRS: {
        switch (piece->rot) {
        case 0:
            extendMineshaft(env, piece->bb0.x, piece->bb0.y, piece->bb0.z - 1, 0, piece->depth);
            break;
        case 1:
            extendMineshaft(env, piece->bb1.x + 1, piece->bb0.y, piece->bb0.z, 1, piece->depth);
            break;
        case 2:
            extendMineshaft(env, piece->bb0.x, piece->bb0.y, piece->bb1.z + 1, 2, piece->depth);
            break;
        case 3:
            extendMineshaft(env, piece->bb0.x - 1, piece->bb0.y, piece->bb0.z, 3, piece->depth);
            break;
        default: UNREACHABLE();
        }
        break;
    }
    default: UNREACHABLE();
    }
}

int getMineshaftPieces(Generator *g, Piece *list, int n, int mc, uint64_t seed, int chunkX, int chunkZ) {
    int x = (chunkX << 4) + 2;
    int z = (chunkZ << 4) + 2;

    uint64_t rng = chunkGenerateRnd(seed, chunkX, chunkZ);
    if (mc >= MC_1_18) nextDouble(&rng);

    entranceCount = 0;
    int count = 1;

    MineshaftPieceEnv env;
    memset(&env, 0, sizeof(env));
    env.mc = mc;
    env.list = list;
    env.n = &count;
    env.rng = &rng;
    env.nmax = n;

    Piece *p = list;
    p->type = MS_ROOM;
    p->name = "MSRoom";

    p->bb0 = p->bb1 = p->pos = (Pos3) {x, 50, z};
    p->bb1.x += 7 + nextInt(&rng, 6);
    p->bb1.y += 4 + nextInt(&rng, 6);
    p->bb1.z += 7 + nextInt(&rng, 6);
    p->depth = 0;
    p->next = NULL;
    p->additionalData = 0;

    extendMineshaftPiece(&env, p);

    int minX = list->bb0.x;
    int minY = list->bb0.y;
    int minZ = list->bb0.z;
    int maxX = list->bb1.x;
    int maxY = list->bb1.y;
    int maxZ = list->bb1.z;
    for (int i = 0; i < count; ++i) {
        Piece *p = &list[i];
        minX = MIN(minX, p->bb0.x);
        minY = MIN(minY, p->bb0.y);
        minZ = MIN(minZ, p->bb0.z);
        maxX = MAX(maxX, p->bb1.x);
        maxY = MAX(maxY, p->bb1.y);
        maxZ = MAX(maxZ, p->bb1.z);
    }

    int vertShift = 0;
    int biome = getBiomeAt(g, 4, chunkX << 2, 0, chunkZ << 2); 
    if (biome == badlands || biome == badlands_plateau || biome == wooded_badlands || biome == wooded_badlands_plateau || biome == eroded_badlands || biome == modified_badlands_plateau || biome == modified_wooded_badlands_plateau) {
        vertShift = 63 - maxY + (maxY - minY + 1) / 2 + 5;
    } else {
        int k = 53;
        int l = maxY - minY + 2; 
        if (l < k) l += nextInt(&rng, (k - l));
        vertShift = l - maxY;
    }    

    minY += vertShift;
    maxY += vertShift;

    for (int i = 0; i < count; ++i) {
        Piece *p = &list[i];
        p->pos.y += vertShift;
        p->bb0.y += vertShift;
        p->bb1.y += vertShift;
    }
    for (int i = 0; i < entranceCount; ++i) {
        entrance1[i].y += vertShift;
        entrance2[i].y += vertShift;
    }

    return count;
}

STRUCT(chunkMask) {
    uint8_t air[8192];
    uint8_t water[8192];
    int cx, cz;
    int16_t topBlock[256];
    uint8_t topBlockValid[256];
    uint8_t details[32768];
};

#define DETAIL_NONE  0
#define DETAIL_AIR   1
#define DETAIL_SOLID 2
#define DETAIL_DECOR 3
#define DETAIL_WATER 4
#define DETAIL_LAVA  5

static inline void setDetails(chunkMask *cm, int x, int y, int z, int v) {
    int lx = x - cm->cx, lz = z - cm->cz;
    if (lx < 0 || lx > 15 || lz < 0 || lz > 15 || y < 0 || y > 255)
        return;
    int idx = (y << 8) | (lz << 4) | lx;
    int sh = (idx & 1) << 2;
    cm->details[idx >> 1] = (cm->details[idx >> 1] & ~(0xF << sh)) | (v << sh);
}

static inline int getDetails(chunkMask *cm, int x, int y, int z) {
    int lx = x - cm->cx, lz = z - cm->cz;
    if (lx < 0 || lx > 15 || lz < 0 || lz > 15 || y < 0 || y > 255)
        return DETAIL_NONE;
    int idx = (y << 8) | (lz << 4) | lx;
    return (cm->details[idx >> 1] >> ((idx & 1) << 2)) & 0xF;
}

static void fillMask(uint8_t *mask, Pos3List *list, int cx, int cz) {
    memset(mask, 0, 8192);
    for (int i = 0; i < list->size; i++) {
        Pos3 a = list->pos3s[i];
        int lx = a.x - cx, lz = a.z - cz;
        if (lx < 0 || lx > 15 || lz < 0 || lz > 15 || a.y < 0 || a.y > 255)
            continue;
        int idx = (a.y << 8) | (lz << 4) | lx;
        mask[idx >> 3] |= 1 << (idx & 7);
    }
}

static inline int getMask(const uint8_t *mask, int cx, int cz, int x, int y, int z) {
    if (y < 0 || y > 255) return 0;
    int lx = x - cx, lz = z - cz;
    if (lx < 0 || lx > 15 || lz < 0 || lz > 15) return 0;
    int idx = (y << 8) | (lz << 4) | lx;
    return (mask[idx >> 3] >> (idx & 7)) & 1;
}

int couldBeNaturalWater(Generator *g, int x, int y, int z);

static int isLiquid(Generator *g, SurfaceNoise *sn, chunkMask *cm, int x, int y, int z) {
    int o = getDetails(cm, x, y, z);
    if (o != DETAIL_NONE) return o == DETAIL_WATER || o == DETAIL_LAVA;
    if (getMask(cm->water, cm->cx, cm->cz, x, y, z)) return y != 10;
    if (getMask(cm->air, cm->cx, cm->cz, x, y, z)) return y <= 10;
    return couldBeNaturalWater(g, x, y, z) && isNaturalWater(g, sn, x, y, z);
}

int touchesLiquid(Generator *g, SurfaceNoise *sn, chunkMask *cm, Piece *p) {
    for (int x = p->bb0.x - 1; x <= p->bb1.x + 1; x++)
        for (int z = p->bb0.z - 1; z <= p->bb1.z + 1; z++)
            if (isLiquid(g, sn, cm, x, p->bb0.y - 1, z) || isLiquid(g, sn, cm, x, p->bb1.y + 1, z))
                return 1;
    for (int x = p->bb0.x - 1; x <= p->bb1.x + 1; x++)
        for (int y = p->bb0.y - 1; y <= p->bb1.y + 1; y++)
            if (isLiquid(g, sn, cm, x, y, p->bb0.z - 1) || isLiquid(g, sn, cm, x, y, p->bb1.z + 1))
                return 1;
    for (int z = p->bb0.z - 1; z <= p->bb1.z + 1; z++)
        for (int y = p->bb0.y - 1; y <= p->bb1.y + 1; y++)
            if (isLiquid(g, sn, cm, p->bb0.x - 1, y, z) || isLiquid(g, sn, cm, p->bb1.x + 1, y, z))
                return 1;
    return 0;
}

static int msTopSolid(Generator *g, const SurfaceNoise *sn, chunkMask *cm, int x, int z);
int couldBeNaturalWater(Generator *g, int x, int y, int z);

// "is the block air" in java terms: overlay from earlier pieces wins, then
// liquid-carved blocks are never air (water/lava/magma), air-carved blocks
// are air only above the lava level, and natural air sits above the terrain
// top from sea level up
static int msIsAirBlock(Generator *g, const SurfaceNoise *sn, chunkMask *cm, int x, int y, int z) {
    int o = getDetails(cm, x, y, z);
    if (o != DETAIL_NONE) return o == DETAIL_AIR;
    if (getMask(cm->water, cm->cx, cm->cz, x, y, z)) return 0;
    if (getMask(cm->air, cm->cx, cm->cz, x, y, z)) return y >= 11;
    if (y >= 63 && y > msTopSolid(g, sn, cm, x, z)) return 1;
    return 0;
}

static inline double msLerp(double part, double from, double to) {
    return from + part * (to - from);
}

// per-corner cell densities for one block column, same math as isNaturalWater
// in generator.c but for the full y range so terrain height can be derived
#define MS_DENS_CELLS 20
static void computeColumnDensity(Generator *g, const SurfaceNoise *sn, int x, int z,
                                double dens[2][2][MS_DENS_CELLS]) {
    int px = x >> 2, pz = z >> 2;
    for (int dx = 0; dx <= 1; dx++)
        for (int dz = 0; dz <= 1; dz++)
            surfaceCornerDens(g, sn, px + dx, pz + dz, dens[dx][dz]);
}

static inline double msColDensAt(const double dens[2][2][MS_DENS_CELLS], int x, int z, int y) {
    int py = y >> 3;
    double fx = (x & 3) / 4.0, fy = (y & 7) / 8.0, fz = (z & 3) / 4.0;
    double l00 = msLerp(fy, dens[0][0][py], dens[0][0][py+1]);
    double l10 = msLerp(fy, dens[1][0][py], dens[1][0][py+1]);
    double l01 = msLerp(fy, dens[0][1][py], dens[0][1][py+1]);
    double l11 = msLerp(fy, dens[1][1][py], dens[1][1][py+1]);
    double lx0 = msLerp(fx, l00, l10);
    double lx1 = msLerp(fx, l01, l11);
    return msLerp(fz, lx0, lx1);
}

// OCEAN_FLOOR_WG heightmap value minus one: the y of the highest
// motion-blocking block after terrain + carvers (water/lava don't block,
// magma from underwater carvers at y==10 does)
static int msTopSolid(Generator *g, const SurfaceNoise *sn, chunkMask *cm, int x, int z) {
    int li = ((z - cm->cz) << 4) | (x - cm->cx);
    if (cm->topBlockValid[li]) return cm->topBlock[li];

    double dens[2][2][MS_DENS_CELLS];
    computeColumnDensity(g, sn, x, z, dens);

    int top = 0;
    for (int y = MS_DENS_CELLS * 8 - 9; y > 0; y--) {
        int o = getDetails(cm, x, y, z);
        if (o != DETAIL_NONE) {
            if (o == DETAIL_SOLID) { top = y; break; }
            continue;
        }
        if (getMask(cm->water, cm->cx, cm->cz, x, y, z)) {
            if (y == 10) { top = y; break; }
            continue;
        }
        if (getMask(cm->air, cm->cx, cm->cz, x, y, z))
            continue;
        if (msColDensAt(dens, x, z, y) > 0) { top = y; break; }
    }
    cm->topBlock[li] = (int16_t)top;
    cm->topBlockValid[li] = 1;
    return top;
}

// java StructurePiece.isInterior: block above (world y) must be strictly
// below the OCEAN_FLOOR_WG heightmap, i.e. <= the top solid block
static inline int msIsInterior(Generator *g, const SurfaceNoise *sn, chunkMask *cm,
                               int x, int yAbove, int z) {
    return yAbove <= msTopSolid(g, sn, cm, x, z);
}

// ---- lake features (java LakeFeature, decoration step LAKES=1) ----
// water/lava lakes generate BEFORE structures (step 3) and their blocks are
// what edgesLiquid & co. see; lakes from the W/N/NW neighbor chunks can spill
// into this chunk if those chunks were decorated earlier

// block kind for the lake sim: 0 air, 1 solid, 2 water, 3 lava
static int msLakeBlockKind(Generator *g, const SurfaceNoise *sn, chunkMask *tgt,
                           chunkMask *src, int x, int y, int z) {
    if (y < 0) return 1;
    if (y > 255) return 0;
    int inTgt = (x >= tgt->cx && x < tgt->cx + 16 && z >= tgt->cz && z < tgt->cz + 16);
    if (inTgt) {
        int o = getDetails(tgt, x, y, z);
        if (o == DETAIL_AIR) return 0;
        if (o == DETAIL_SOLID || o == DETAIL_DECOR) return 1;
        if (o == DETAIL_WATER) return 2;
        if (o == DETAIL_LAVA) return 3;
    }
    chunkMask *m = inTgt ? tgt : src;
    if (m && x >= m->cx && x < m->cx + 16 && z >= m->cz && z < m->cz + 16) {
        if (getMask(m->water, m->cx, m->cz, x, y, z))
            return y == 10 ? 1 : (y < 10 ? 3 : 2);
        if (getMask(m->air, m->cx, m->cz, x, y, z))
            return y >= 11 ? 0 : 3;
    }
    // natural terrain (small per-column cache)
    {
        static uint64_t cSeed[4]; static int cX[4], cZ[4], cValid[4], cNext;
        static double cDens[4][2][2][MS_DENS_CELLS];
        int slot = -1;
        for (int i = 0; i < 4; i++)
            if (cValid[i] && cSeed[i] == g->seed && cX[i] == x && cZ[i] == z) { slot = i; break; }
        if (slot < 0) {
            slot = cNext; cNext = (cNext + 1) % 4;
            computeColumnDensity(g, sn, x, z, cDens[slot]);
            cSeed[slot] = g->seed; cX[slot] = x; cZ[slot] = z; cValid[slot] = 1;
        }
        if (y >= MS_DENS_CELLS * 8 - 8) return y < 63 ? 2 : 0;
        if (msColDensAt(cDens[slot], x, z, y) > 0) return 1;
        return y < 63 ? 2 : 0;
    }
}

// simulate the two lake features of source chunk (scx16,scz16 block base) and
// write any blob blocks landing in the target chunk into its overlay
static void msSimLakesInto(Generator *g, SurfaceNoise *sn, chunkMask *tgt, chunkMask *src,
                           int mc, uint64_t seed, int scx16, int scz16) {
    int biome = getBiomeAt(g, 4, ((scx16 >> 4) << 2) + 2, 2, ((scz16 >> 4) << 2) + 2);
    int waterIdx = 0, lavaIdx = 1;
    if (biome == desert || biome == desert_hills || biome == desert_lakes) {
        waterIdx = -1; lavaIdx = 0;
    }
    uint64_t pop = getPopulationSeed(mc, seed, scx16, scz16);

    for (int f = 0; f < 2; f++) {
        int isLava = f;
        int idx = isLava ? lavaIdx : waterIdx;
        if (idx < 0) continue;
        uint64_t r;
        setSeed(&r, pop + 10000 * 1 + idx);
        int ox, oz, oy;
        if (!isLava) {
            if (nextInt(&r, 4) != 0) continue;
            ox = nextInt(&r, 16) + scx16;
            oz = nextInt(&r, 16) + scz16;
            oy = nextInt(&r, 256);
        } else {
            if (nextInt(&r, 8) != 0) continue;
            ox = nextInt(&r, 16) + scx16;
            oz = nextInt(&r, 16) + scz16;
            oy = nextInt(&r, nextInt(&r, 248) + 8);
            if (!(oy < 63 || nextInt(&r, 10) == 0)) continue;
        }

        // LakeFeature.place: descend through air
        while (oy > 5 && msLakeBlockKind(g, sn, tgt, src, ox, oy, oz) == 0) oy--;
        if (oy <= 4) continue;
        oy -= 4;

        char bls[2048];
        memset(bls, 0, sizeof(bls));
        int nb = nextInt(&r, 4) + 4;
        for (int b = 0; b < nb; b++) {
            double d  = nextDouble(&r) * 6.0 + 3.0;
            double e  = nextDouble(&r) * 4.0 + 2.0;
            double fw = nextDouble(&r) * 6.0 + 3.0;
            double gx = nextDouble(&r) * (16.0 - d - 2.0) + 1.0 + d / 2.0;
            double h  = nextDouble(&r) * (8.0 - e - 4.0) + 2.0 + e / 2.0;
            double k  = nextDouble(&r) * (16.0 - fw - 2.0) + 1.0 + fw / 2.0;
            for (int l = 1; l < 15; l++)
            for (int m = 1; m < 15; m++)
            for (int n = 1; n < 7; n++) {
                double o = (l - gx) / (d / 2.0);
                double p = (n - h) / (e / 2.0);
                double q = (m - k) / (fw / 2.0);
                if (o*o + p*p + q*q < 1.0)
                    bls[(l*16 + m)*8 + n] = 1;
            }
        }

        int ok = 1;
        for (int j = 0; j < 16 && ok; j++)
        for (int s2 = 0; s2 < 16 && ok; s2++)
        for (int t = 0; t < 8 && ok; t++) {
            int edge = !bls[(j*16 + s2)*8 + t] && (
                (j < 15 && bls[((j+1)*16 + s2)*8 + t]) || (j > 0 && bls[((j-1)*16 + s2)*8 + t]) ||
                (s2 < 15 && bls[(j*16 + s2 + 1)*8 + t]) || (s2 > 0 && bls[(j*16 + s2 - 1)*8 + t]) ||
                (t < 7 && bls[(j*16 + s2)*8 + t + 1]) || (t > 0 && bls[(j*16 + s2)*8 + t - 1]));
            if (!edge) continue;
            int kind = msLakeBlockKind(g, sn, tgt, src, ox + j, oy + t, oz + s2);
            if (t >= 4 && (kind == 2 || kind == 3)) ok = 0;
            else if (t < 4 && kind != 1 && kind != (isLava ? 3 : 2)) ok = 0;
        }
        if (!ok) continue;

        for (int j = 0; j < 16; j++)
        for (int s2 = 0; s2 < 16; s2++)
        for (int t = 0; t < 8; t++)
            if (bls[(j*16 + s2)*8 + t])
                setDetails(tgt, ox + j, oy + t, oz + s2,
                         t >= 4 ? DETAIL_AIR : (isLava ? DETAIL_LAVA : DETAIL_WATER));
    }
}

// per-structure cache of applyAllCarvers results: every chunk in the pass
// list gets carved once and reused (main pass + up to 4 lake-neighbor uses)
STRUCT(MsCarverCache) {
    Pos3List air, water;
    int valid;
};

static void carveChunk(Generator *g, SurfaceNoise *sn, MsCarverCache *cc, int cx16, int cz16) {
    if (!cc->valid) {
        createPos3List(&cc->air, 1);
        createPos3List(&cc->water, 1);
        applyAllCarvers(g, sn, cx16 >> 4, cz16 >> 4, &cc->air, &cc->water);
        cc->valid = 1;
    }
}

static void applyLakes(Generator *g, SurfaceNoise *sn, chunkMask *tgt, int mc, uint64_t seed,
                         int *chunkXs, int *chunkZs, int nchunks, int ci, MsCarverCache *carverCache) {
    // sources whose blob (origin+15) can reach this chunk: itself, W, N, NW --
    // but a neighbor's lake only exists if that chunk was decorated earlier
    int candX[4], candZ[4], candIdx[4];
    int nc = 0;
    static const int offs[4][2] = {{-16,-16},{-16,0},{0,-16},{0,0}};
    for (int c = 0; c < 4; c++) {
        int sx = tgt->cx + offs[c][0], sz = tgt->cz + offs[c][1];
        int idx = -1;
        if (sx == tgt->cx && sz == tgt->cz) {
            idx = ci; // own lakes always precede structures (step 1 < 3)
        } else {
            for (int q = 0; q < nchunks; q++)
                if (chunkXs[q] == sx && chunkZs[q] == sz) { idx = q; break; }
            if (idx < 0 || idx > ci) continue; // not decorated yet
        }
        candX[nc] = sx; candZ[nc] = sz; candIdx[nc] = idx; nc++;
    }
    // apply in decoration order
    for (int a = 1; a < nc; a++) {
        int kx = candX[a], kz = candZ[a], ki = candIdx[a];
        int b = a - 1;
        while (b >= 0 && candIdx[b] > ki) {
            candX[b+1] = candX[b]; candZ[b+1] = candZ[b]; candIdx[b+1] = candIdx[b];
            b--;
        }
        candX[b+1] = kx; candZ[b+1] = kz; candIdx[b+1] = ki;
    }
    for (int c = 0; c < nc; c++) {
        if (candX[c] == tgt->cx && candZ[c] == tgt->cz) {
            msSimLakesInto(g, sn, tgt, tgt, mc, seed, candX[c], candZ[c]);
        } else {
            chunkMask *src = (chunkMask*)malloc(sizeof(chunkMask));
            src->cx = candX[c];
            src->cz = candZ[c];
            MsCarverCache *cc = &carverCache[candIdx[c]];
            carveChunk(g, sn, cc, candX[c], candZ[c]);
            fillMask(src->air, &cc->air, candX[c], candZ[c]);
            fillMask(src->water, &cc->water, candX[c], candZ[c]);
            msSimLakesInto(g, sn, tgt, src, mc, seed, candX[c], candZ[c]);
            free(src);
        }
    }
}

static int isSupportingBox(Generator *g, const SurfaceNoise *sn, Piece *p, int x0, int x1, int z0, chunkMask *cm) {
    int ceilY = p->bb0.y + 3;
    for (int x = x0; x <= x1; x++) {
        int tx = x, tz = z0;
        rotPos(p->bb0, p->bb1, &tx, &tz, p->rot);
        if (tx < cm->cx || tx >= cm->cx + 16 || tz < cm->cz || tz >= cm->cz + 16) {
            return 0;
        }
        if (msIsAirBlock(g, sn, cm, tx, ceilY, tz)) {
            return 0;
        }
    }
    return 1;
}

// mark a piece-local block in the overlay if it lands in the current chunk
static void msOvlSetLocal(chunkMask *cm, Piece *p, int x, int y, int z, int v) {
    rotPos(p->bb0, p->bb1, &x, &z, p->rot);
    setDetails(cm, x, p->bb0.y + y, z, v);
}

static void placeSupport(Generator *g, const SurfaceNoise *sn, Piece *p, int x0, int z, int x1,
                         RandomSource rnd, chunkMask *cm) {
    int sup = isSupportingBox(g, sn, p, x0, x1, z, cm);
    if (sup) {
        for (int yy = 0; yy <= 1; yy++) {
            msOvlSetLocal(cm, p, x0, yy, z, DETAIL_DECOR);   // fence columns
            msOvlSetLocal(cm, p, x1, yy, z, DETAIL_DECOR);
        }
        if (rnd.nextInt(rnd.state, 4) == 0) {
            msOvlSetLocal(cm, p, x0, 2, z, DETAIL_SOLID);    // plank caps
            msOvlSetLocal(cm, p, x1, 2, z, DETAIL_SOLID);
        } else {
            for (int xx = x0; xx <= x1; xx++)
                msOvlSetLocal(cm, p, xx, 2, z, DETAIL_SOLID); // plank beam
            if (rnd.nextFloat(rnd.state) < 0.05f)
                msOvlSetLocal(cm, p, x0 + 1, 2, z - 1, DETAIL_DECOR); // torch
            if (rnd.nextFloat(rnd.state) < 0.05f)
                msOvlSetLocal(cm, p, x0 + 1, 2, z + 1, DETAIL_DECOR); // torch
        }
    }
}

// java placeCobWeb: the roll itself is gated on isInterior (heightmap check
// on the block above, world y = bb0.y + 3 for the corridor cobwebs at local y=2)
static void maybePlaceCobWeb(Generator *g, const SurfaceNoise *sn, chunkMask *cm, Piece *p, RandomSource rnd, float chance, int x, int z) {
    int tx = x, tz = z;
    rotPos(p->bb0, p->bb1, &tx, &tz, p->rot);
    if (tx >= cm->cx && tx < cm->cx + 16 && tz >= cm->cz && tz < cm->cz + 16 &&
        msIsInterior(g, sn, cm, tx, p->bb0.y + 3, tz)) {
        if (rnd.nextFloat(rnd.state) < chance)
            setDetails(cm, tx, p->bb0.y + 2, tz, DETAIL_DECOR);
    }
}

// cached quart biomes (biomes are 4:1 and getBiomeAt is expensive); the
// cache is a repeating 256x256 tile of the quart grid: a position's slot
// is just the low 8 bits of each coordinate, so two positions only share
// a slot when they are an exact multiple of 256 quarts (1024 blocks)
// apart. The stored seed/px/pz are checked before a slot is trusted, so
// a collision or stale entry just recomputes and can never be wrong.
static uint64_t msBiomeCacheSeed[256 * 256];
static int msBiomeCachePx[256 * 256];
static int msBiomeCachePz[256 * 256];
static int msBiomeCacheId[256 * 256];
static uint8_t msBiomeCacheValid[256 * 256];

int msLookupBiome(Generator *g, int px, int pz) {
    uint32_t i = (px & 255) | ((pz & 255) << 8);
    if (msBiomeCacheValid[i] && msBiomeCacheSeed[i] == g->seed &&
        msBiomeCachePx[i] == px && msBiomeCachePz[i] == pz)
        return msBiomeCacheId[i];

    int id = getBiomeAt(g, 4, px, 0, pz);
    msBiomeCacheSeed[i] = g->seed;
    msBiomeCachePx[i] = px;
    msBiomeCachePz[i] = pz;
    msBiomeCacheId[i] = id;
    msBiomeCacheValid[i] = 1;
    return id;
}

int couldBeNaturalWater(Generator *g, int x, int y, int z) { // tried to optimize via preliminary check
    if (y >= 63 || y < 0)
        return 0;

    int id = msLookupBiome(g, x >> 2, z >> 2);
    double depth, scale;
    getBiomeDepthAndScale(id, &depth, &scale, 0);

    scale = scale * 0.9 + 0.1;
    depth = (depth * 4.0 - 1) / 8;
    scale = 96 / scale;
    depth = depth * 17. / 64;

    int py = y >> 3;
    double worstCaseDensity = 1e300;
    int k;
    for (k = 0; k <= 1; k++) {
        double fall = 1 - 2 * (py + k) / 32.0 + (-2.0 * 17./64 / 28.) - 0.46875;
        fall = scale * (fall + depth);
        double density = (fall > 0 ? 4*fall : fall) - 8.0;
        if (density < worstCaseDensity)
            worstCaseDensity = density;
    }
    return worstCaseDensity <= 0;
}

int getMineshaftLoot(Generator *g, SurfaceNoise *sn, Piece *list, int n, StructureSaltConfig ssconf, int mc, uint64_t seed, int chunkX, int chunkZ) {
    int count = getMineshaftPieces(g, list, n, mc, seed, chunkX, chunkZ);

    const int legacy = mc <= MC_1_17;
    int minX = list->bb0.x;
    int minZ = list->bb0.z;
    int maxX = list->bb1.x;
    int maxZ = list->bb1.z;
    for (int i = 0; i < count; ++i) {
        Piece *p = &list[i];
        minX = MIN(minX, p->bb0.x);
        minZ = MIN(minZ, p->bb0.z);
        maxX = MAX(maxX, p->bb1.x);
        maxZ = MAX(maxZ, p->bb1.z);

        p->chestCount = 0;
    }
    int cMinX = (minX - 1) & ~15;
    int cMinZ = (minZ - 1) & ~15;
    int cMaxX = (maxX + 1) & ~15;
    int cMaxZ = (maxZ + 1) & ~15;

    // chunks are processed in order of squared distance from the structure's
    // start chunk: piece generation is chunk-order dependent in vanilla (a
    // piece that hits liquid in one chunk stops generating in all later
    // chunks), and this radial order matches worlds whose chunks generate
    // outward from the structure (e.g. /tp to it in an unexplored area)
    int maxChunks = ((cMaxX - cMinX) / 16 + 1) * ((cMaxZ - cMinZ) / 16 + 1);
    int *chunkXs = (int*)malloc(maxChunks * sizeof(int));
    int *chunkZs = (int*)malloc(maxChunks * sizeof(int));
    int nchunks = 0;
    for (int cx = cMinX; cx <= cMaxX; cx += 16)
        for (int cz = cMinZ; cz <= cMaxZ; cz += 16) {
            chunkXs[nchunks] = cx;
            chunkZs[nchunks] = cz;
            nchunks++;
        }
    for (int a = 1; a < nchunks; a++) {
        int keyX = chunkXs[a], keyZ = chunkZs[a];
        int kdx = (keyX >> 4) - chunkX, kdz = (keyZ >> 4) - chunkZ;
        int kd = kdx*kdx + kdz*kdz;
        int b = a - 1;
        while (b >= 0) {
            int bdx = (chunkXs[b] >> 4) - chunkX, bdz = (chunkZs[b] >> 4) - chunkZ;
            if (bdx*bdx + bdz*bdz <= kd) break;
            chunkXs[b+1] = chunkXs[b];
            chunkZs[b+1] = chunkZs[b];
            b--;
        }
        chunkXs[b+1] = keyX;
        chunkZs[b+1] = keyZ;
    }
    int *removed = (int*)calloc(count, sizeof(int));
    MsCarverCache *carverCache = (MsCarverCache*)calloc(nchunks, sizeof(MsCarverCache));

    for (int ci = 0; ci < nchunks; ci++) {
        int cx = chunkXs[ci], cz = chunkZs[ci];
        CREATE_RANDOM_SOURCE(rnd, legacy);
        uint64_t populationSeed = getPopulationSeed(mc, seed, cx, cz);
        rnd.setSeed(rnd.state, populationSeed + ssconf.generationStep * 10000 + ssconf.decoratorIndex);

        carveChunk(g, sn, &carverCache[ci], cx, cz);

        chunkMask cm;
        cm.cx = cx;
        cm.cz = cz;
        fillMask(cm.air, &carverCache[ci].air, cx, cz);
        fillMask(cm.water, &carverCache[ci].water, cx, cz);
        memset(cm.topBlockValid, 0, sizeof(cm.topBlockValid));
        memset(cm.details, 0, sizeof(cm.details));
        applyLakes(g, sn, &cm, mc, seed, chunkXs, chunkZs, nchunks, ci, carverCache);

        for (int i = 0; i < count; ++i) {
            Piece *p = &list[i];
            if (removed[i]) continue;
            if (!(p->bb1.x >= cx && p->bb0.x <= cx + 15 &&
                  p->bb1.z >= cz && p->bb0.z <= cz + 15)) {
                continue;
            }

            if (touchesLiquid(g, sn, &cm, p)) {
                removed[i] = 1;
                continue;
            }

            switch (p->type) {
                case MS_CORRIDOR: {
                    // isInInvalidLocation ignored
                    int numSections;
                    if (p->rot == 0 || p->rot == 2) {
                        numSections = (p->bb1.z - p->bb0.z + 1) / 5;
                    } else {
                        numSections = (p->bb1.x - p->bb0.x + 1) / 5;
                    }
                    int length = numSections * 5 - 1;

                    // unconditional air fill (0..2, 0..1, 0..length), no RNG
                    for (int yy = 0; yy <= 1; yy++)
                    for (int xx = 0; xx <= 2; xx++)
                    for (int zz = 0; zz <= length; zz++)
                        msOvlSetLocal(&cm, p, xx, yy, zz, DETAIL_AIR);

                    // 0.8F cave-air box on the ceiling row (y=2), roll per cell
                    for (int xx = 0; xx <= 2; xx++)
                    for (int zz = 0; zz <= length; zz++) {
                        if (rnd.nextFloat(rnd.state) <= 0.8f)
                            msOvlSetLocal(&cm, p, xx, 2, zz, DETAIL_AIR);
                    }

                    // spider corridors roll a 0.6F cobweb box over (0..2, 0..1, 0..length)
                    // before the chest rolls; cobwebs landing on a chest cell make java's
                    // createChest fail its isAir condition (cell order in java: y, x, z)
                    if ((p->additionalData >> 1) & 1) {
                        for (int yy = 0; yy <= 1; yy++)
                        for (int xx = 0; xx <= 2; xx++)
                        for (int zz = 0; zz <= length; zz++) {
                            float f = rnd.nextFloat(rnd.state);
                            if (f > 0.6f) continue;
                            int tx = xx, tz = zz;
                            rotPos(p->bb0, p->bb1, &tx, &tz, p->rot);
                            if (tx >= cx && tx < cx + 16 && tz >= cz && tz < cz + 16 &&
                                msIsInterior(g, sn, &cm, tx, p->bb0.y + yy + 1, tz)) {
                                setDetails(&cm, tx, p->bb0.y + yy, tz, DETAIL_DECOR);
                            }
                        }
                    }

                    for (int section = 0; section < numSections; section++) {
                        int z = 2 + section * 5;
                        placeSupport(g, sn, p, 0, z, 2, rnd, &cm);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.1f, 0, z - 1);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.1f, 2, z - 1);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.1f, 0, z + 1);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.1f, 2, z + 1);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.05f, 0, z - 2);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.05f, 2, z - 2);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.05f, 0, z + 2);
                        maybePlaceCobWeb(g, sn, &cm, p, rnd, 0.05f, 2, z + 2);

                        for (int side = 0; side < 2; side++) {
                            int roll = rnd.nextInt(rnd.state, 100);
                            if (roll != 0) continue;
                            int lcx = side == 0 ? 2 : 0;
                            int lcz = side == 0 ? z - 1 : z + 1;
                            int chestPosX = lcx, chestPosZ = lcz;
                            rotPos(p->bb0, p->bb1, &chestPosX, &chestPosZ, p->rot);
                            int inChunk = chestPosX >= cx && chestPosX < cx + 16 &&
                                          chestPosZ >= cz && chestPosZ < cz + 16;
                            int posAir = inChunk && msIsAirBlock(g, sn, &cm, chestPosX, p->bb0.y, chestPosZ);
                            int floorAir = inChunk && msIsAirBlock(g, sn, &cm, chestPosX, p->bb0.y - 1, chestPosZ);
                            if (inChunk && posAir && !floorAir) {
                                rnd.nextBoolean(rnd.state);
                                p->chestPoses[p->chestCount] = (Pos) {chestPosX, chestPosZ};
                                p->lootTables[p->chestCount] = "abandoned_mineshaft";
                                p->lootSeeds[p->chestCount] = rnd.nextLong(rnd.state);
                                p->chestCount++;
                                setDetails(&cm, chestPosX, p->bb0.y, chestPosZ, DETAIL_DECOR); // rail under minecart
                            }
                        }

                        // this.spiderCorridor && !this.hasPlacedSpider
                        // spiderCorridor is 0b_X_, hasPlacedSpider is 0bX__
                        if (((p->additionalData >> 1) & 0b11) == 0b01) {
                            int newX = 1;
                            int spiderRoll = rnd.nextInt(rnd.state, 3);
                            int newZ = z - 1 + spiderRoll;
                            rotPos(p->bb0, p->bb1, &newX, &newZ, p->rot);
                            if (newX >= cx && newX < cx + 16 && newZ >= cz && newZ < cz + 16 &&
                                msIsInterior(g, sn, &cm, newX, p->bb0.y + 1, newZ)) {
                                p->additionalData |= 1 << 2;
                                setDetails(&cm, newX, p->bb0.y, newZ, DETAIL_SOLID); // spawner
                            }
                        }
                    }

                    // floor planks: air floors inside the footprint get planks when interior
                    for (int xx = 0; xx <= 2; xx++)
                    for (int zz = 0; zz <= length; zz++) {
                        int tx = xx, tz = zz;
                        rotPos(p->bb0, p->bb1, &tx, &tz, p->rot);
                        if (tx >= cx && tx < cx + 16 && tz >= cz && tz < cz + 16 &&
                            msIsAirBlock(g, sn, &cm, tx, p->bb0.y - 1, tz) &&
                            msIsInterior(g, sn, &cm, tx, p->bb0.y, tz)) {
                            setDetails(&cm, tx, p->bb0.y - 1, tz, DETAIL_SOLID);
                        }
                    }

                    // this.hasRails: roll iff the floor block is !air && solidRender
                    if ((p->additionalData >> 0) & 1) {
                        int floorY = p->bb0.y - 1;
                        for (int zx = 0; zx <= length; zx++) {
                            int tx = 1, tz = zx;
                            rotPos(p->bb0, p->bb1, &tx, &tz, p->rot);
                            if (tx >= cx && tx < cx + 16 && tz >= cz && tz < cz + 16) {
                                int rolls;
                                int fo = getDetails(&cm, tx, floorY, tz);
                                if (fo != DETAIL_NONE) {
                                    rolls = (fo == DETAIL_SOLID);
                                } else if (getMask(cm.water, cx, cz, tx, floorY, tz)) {
                                    rolls = (floorY == 10);  // magma is solid, water/lava not
                                } else if (getMask(cm.air, cx, cz, tx, floorY, tz)) {
                                    rolls = 0;  // still air here means non-interior (no plank) or lava
                                } else if (couldBeNaturalWater(g, tx, floorY, tz) &&
                                           isNaturalWater(g, sn, tx, floorY, tz)) {
                                    rolls = 0;
                                } else if (floorY >= 63 && floorY > msTopSolid(g, sn, &cm, tx, tz)) {
                                    rolls = 0;  // natural air floor
                                } else {
                                    rolls = 1;
                                }
                                if (rolls) {
                                    float rv = rnd.nextFloat(rnd.state);
                                    float chance = msIsInterior(g, sn, &cm, tx, p->bb0.y + 1, tz) ? 0.7f : 0.9f;
                                    if (rv < chance)
                                        setDetails(&cm, tx, p->bb0.y, tz, DETAIL_DECOR); // rail
                                }
                            }
                        }
                    }
                    break;
                }
                case MS_CROSSING: {
                    // air fills (absolute coords in java, crossing has no orientation)
                    int x0 = p->bb0.x, y0 = p->bb0.y, z0 = p->bb0.z;
                    int x1 = p->bb1.x, y1 = p->bb1.y, z1 = p->bb1.z;
                    int twoFloored = (y1 - y0 + 1) > 3;
                    if (twoFloored) {
                        for (int x = x0+1; x <= x1-1; x++) for (int y = y0; y <= y0+2; y++) for (int z = z0; z <= z1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                        for (int x = x0; x <= x1; x++) for (int y = y0; y <= y0+2; y++) for (int z = z0+1; z <= z1-1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                        for (int x = x0+1; x <= x1-1; x++) for (int y = y1-2; y <= y1; y++) for (int z = z0; z <= z1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                        for (int x = x0; x <= x1; x++) for (int y = y1-2; y <= y1; y++) for (int z = z0+1; z <= z1-1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                        for (int x = x0+1; x <= x1-1; x++) for (int z = z0+1; z <= z1-1; z++) setDetails(&cm, x, y0+3, z, DETAIL_AIR);
                    } else {
                        for (int x = x0+1; x <= x1-1; x++) for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                        for (int x = x0; x <= x1; x++) for (int y = y0; y <= y1; y++) for (int z = z0+1; z <= z1-1; z++) setDetails(&cm, x, y, z, DETAIL_AIR);
                    }
                    // support pillars: placed when the block above the crossing is not air
                    int pxs[4] = {x0+1, x0+1, x1-1, x1-1};
                    int pzs[4] = {z0+1, z1-1, z0+1, z1-1};
                    for (int pi = 0; pi < 4; pi++) {
                        int px = pxs[pi], pz = pzs[pi];
                        if (px >= cx && px < cx + 16 && pz >= cz && pz < cz + 16 &&
                            !msIsAirBlock(g, sn, &cm, px, y1 + 1, pz)) {
                            for (int y = y0; y <= y1; y++)
                                setDetails(&cm, px, y, pz, DETAIL_SOLID);
                        }
                    }
                    // floor planks
                    for (int x = x0; x <= x1; x++)
                    for (int z = z0; z <= z1; z++) {
                        if (x >= cx && x < cx + 16 && z >= cz && z < cz + 16 &&
                            msIsAirBlock(g, sn, &cm, x, y0 - 1, z) &&
                            msIsInterior(g, sn, &cm, x, y0, z)) {
                            setDetails(&cm, x, y0 - 1, z, DETAIL_SOLID);
                        }
                    }
                    break;
                }
                case MS_ROOM: {
                    int x0 = p->bb0.x, y0 = p->bb0.y, z0 = p->bb0.z;
                    int x1 = p->bb1.x, y1 = p->bb1.y, z1 = p->bb1.z;
                    // dirt floor, only replacing blocks that are not currently air
                    for (int x = x0; x <= x1; x++)
                    for (int z = z0; z <= z1; z++) {
                        if (x >= cx && x < cx + 16 && z >= cz && z < cz + 16 &&
                            !msIsAirBlock(g, sn, &cm, x, y0, z)) {
                            setDetails(&cm, x, y0, z, DETAIL_SOLID);
                        }
                    }
                    // interior air
                    int yTop = MIN(y0 + 3, y1);
                    for (int x = x0; x <= x1; x++)
                    for (int y = y0 + 1; y <= yTop; y++)
                    for (int z = z0; z <= z1; z++)
                        setDetails(&cm, x, y, z, DETAIL_AIR);
                    // entrance cutouts recorded during piece generation
                    for (int e = 0; e < entranceCount; e++) {
                        for (int x = entrance1[e].x; x <= entrance2[e].x; x++)
                        for (int y = entrance2[e].y - 2; y <= entrance2[e].y; y++)
                        for (int z = entrance1[e].z; z <= entrance2[e].z; z++)
                            setDetails(&cm, x, y, z, DETAIL_AIR);
                    }
                    // upper half sphere of air
                    if (y0 + 4 <= y1) {
                        float fx = x1 - x0 + 1, fg = y1 - (y0+4) + 1, fh = z1 - z0 + 1;
                        float ci = x0 + fx / 2.0f, cj = z0 + fh / 2.0f;
                        for (int y = y0 + 4; y <= y1; y++) {
                            float l = (y - (y0+4)) / fg;
                            for (int x = x0; x <= x1; x++) {
                                float nn = (x - ci) / (fx * 0.5f);
                                for (int z = z0; z <= z1; z++) {
                                    float pp = (z - cj) / (fh * 0.5f);
                                    if (nn*nn + l*l + pp*pp <= 1.05f)
                                        setDetails(&cm, x, y, z, DETAIL_AIR);
                                }
                            }
                        }
                    }
                    break;
                }
                case MS_STAIRS: {
                    for (int yy = 5; yy <= 7; yy++)
                    for (int xx = 0; xx <= 2; xx++)
                    for (int zz = 0; zz <= 1; zz++)
                        msOvlSetLocal(&cm, p, xx, yy, zz, DETAIL_AIR);
                    for (int yy = 0; yy <= 2; yy++)
                    for (int xx = 0; xx <= 2; xx++)
                    for (int zz = 7; zz <= 8; zz++)
                        msOvlSetLocal(&cm, p, xx, yy, zz, DETAIL_AIR);
                    for (int st = 0; st < 5; st++) {
                        int yLo = 5 - st - (st < 4 ? 1 : 0);
                        int yHi = 7 - st;
                        for (int yy = yLo; yy <= yHi; yy++)
                        for (int xx = 0; xx <= 2; xx++)
                            msOvlSetLocal(&cm, p, xx, yy, 2 + st, DETAIL_AIR);
                    }
                    break;
                }
                default: UNREACHABLE();
                }
            }
        }

    for (int q = 0; q < nchunks; q++) {
        if (carverCache[q].valid) {
            freePos3List(&carverCache[q].air);
            freePos3List(&carverCache[q].water);
        }
    }
    free(carverCache);
    free(chunkXs);
    free(chunkZs);
    free(removed);
    return count;
}
