#ifndef DUNGEON_H_
#define DUNGEON_H_

#include "../finders.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*DungeonBlockFxn)(void *ctx, int x, int y, int z, int wantAir);

typedef void (*DungeonSetFxn)(void *ctx, int x, int y, int z, int kind);

STRUCT(DungeonRoom) {
    Pos3 spawnerPos;
    int halfX, halfZ; // room half widths (2 or 3)
    int chestCount;
    Pos3 chestPoses[2];
    uint64_t chestLootSeeds[2];
};

STRUCT(DungeonRoomList) {
    DungeonRoom *rooms;
    int size;
    int capacity;
};

void createDungeonRoomList(DungeonRoomList *list, int initialCapacity);
void appendDungeonRoom(DungeonRoomList *list, DungeonRoom room);
void freeDungeonRoomList(DungeonRoomList *list);

int simMonsterRooms(int mc, uint64_t worldSeed, int chunkX, int chunkZ, int featureIndex, DungeonBlockFxn getBlock, DungeonSetFxn setBlock, void *ctx, DungeonRoomList *roomsOut);

// TODO add standalone function to find monster spawners (requries carvers and structures to be simmed first so itll probably be very slow)

#ifdef __cplusplus
}
#endif

#endif // DUNGEON_H_
