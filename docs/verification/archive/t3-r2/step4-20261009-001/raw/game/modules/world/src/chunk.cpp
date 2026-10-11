#include "chunk.h"
#include <symocraft/world/constants.h>

namespace SymoCraft::World::Detail {
    Chunk::Chunk(ChunkCoord position, bool edge) : coordinate(position), fringe(edge),
        blocks(static_cast<std::size_t>(16 * 16 * 256), BlockConstants::AIR_BLOCK) {}
}
