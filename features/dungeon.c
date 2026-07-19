#include "dungeon.h"
#include "../rng.h"

#include <stdlib.h>

void createDungeonRoomList(DungeonRoomList *list, int initialCapacity) {
    list->rooms = (DungeonRoom*)malloc(initialCapacity * sizeof(DungeonRoom));
    list->size = 0;
    list->capacity = initialCapacity;
}

void appendDungeonRoom(DungeonRoomList *list, DungeonRoom room) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->rooms = (DungeonRoom*)realloc(list->rooms, list->capacity * sizeof(DungeonRoom));
    }
    list->rooms[list->size++] = room;
}

void freeDungeonRoomList(DungeonRoomList *list) {
    free(list->rooms);
    list->rooms = NULL;
    list->size = list->capacity = 0;
}

static int placeMonsterRoom(uint64_t *rnd, int x, int y, int z, DungeonBlockFxn getBlock, DungeonSetFxn setBlock, void *ctx, DungeonRoom *room) {
    int j = nextInt(rnd, 2) + 2;
    int k = -j - 1, l = j + 1;
    int o = nextInt(rnd, 2) + 2;
    int p = -o - 1, q = o + 1;

    int openings = 0;
    for (int s = k; s <= l; s++) {
        for (int t = -1; t <= 4; t++) {
            for (int u = p; u <= q; u++) {
                int bx = x + s, by = y + t, bz = z + u;
                int solid = by >= 0 && by <= 255 && getBlock(ctx, bx, by, bz, 0);
                if ((t == -1 || t == 4) && !solid) return 0;
                if ((s == k || s == l || u == p || u == q) && t == 0 && getBlock(ctx, bx, by, bz, 1) && getBlock(ctx, bx, by + 1, bz, 1)) openings++;
            }
        }
    }
    if (openings < 1 || openings > 5) return 0;

    for (int s = k; s <= l; s++) {
        for (int t = 3; t >= -1; t--) {
            for (int u = p; u <= q; u++) {
                int bx = x + s, by = y + t, bz = z + u;
                if (s != k && t != -1 && u != p && s != l && t != 4 && u != q) {
                    setBlock(ctx, bx, by, bz, 1);
                } else if (by >= 0 && !(by - 1 >= 0 && getBlock(ctx, bx, by - 1, bz, 0))) {
                    setBlock(ctx, bx, by, bz, 1);
                } else if (getBlock(ctx, bx, by, bz, 0)) {
                    if (t == -1)
                        nextInt(rnd, 4); // mossy vs plain cobblestone
                    setBlock(ctx, bx, by, bz, 2);
                }
            }
        }
    }

    room->spawnerPos = (Pos3) {x, y, z};
    room->halfX = j;
    room->halfZ = o;
    room->chestCount = 0;
    for (int a = 0; a < 2; a++) {
        for (int b = 0; b < 3; b++) {
            int cxp = x + nextInt(rnd, j * 2 + 1) - j;
            int czp = z + nextInt(rnd, o * 2 + 1) - o;
            if (!getBlock(ctx, cxp, y, czp, 1)) continue;
            int solidSides = 0;
            solidSides += getBlock(ctx, cxp, y, czp - 1, 0);
            solidSides += getBlock(ctx, cxp, y, czp + 1, 0);
            solidSides += getBlock(ctx, cxp - 1, y, czp, 0);
            solidSides += getBlock(ctx, cxp + 1, y, czp, 0);
            if (solidSides == 1) {
                setBlock(ctx, cxp, y, czp, 3);
                room->chestPoses[room->chestCount] = (Pos3) {cxp, y, czp};
                room->chestLootSeeds[room->chestCount] = nextLong(rnd);
                room->chestCount++;
                break;
            }
        }
    }

    setBlock(ctx, x, y, z, 3);
    nextInt(rnd, 4); // spawner mob type
    return 1;
}

int simMonsterRooms(int mc, uint64_t worldSeed, int chunkX, int chunkZ, int featureIndex, DungeonBlockFxn getBlock, DungeonSetFxn setBlock, void *ctx, DungeonRoomList *roomsOut) {
    uint64_t rnd;
    uint64_t popSeed = getPopulationSeed(mc, worldSeed, chunkX << 4, chunkZ << 4);
    setSeed(&rnd, popSeed + featureIndex + 10000 * 3); // underground structures
    int placed = 0;
    for (int attempt = 0; attempt < 8; attempt++) {
        int x = (chunkX << 4) + nextInt(&rnd, 16);
        int z = (chunkZ << 4) + nextInt(&rnd, 16);
        int y = nextInt(&rnd, 256);
        DungeonRoom room;
        if (placeMonsterRoom(&rnd, x, y, z, getBlock, setBlock, ctx, &room)) {
            if (roomsOut)
                appendDungeonRoom(roomsOut, room);
            placed++;
        }
    }
    return placed;
}
