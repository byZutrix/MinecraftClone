#include "block.h"

bool Block::isAir() const {
    if (this == nullptr) return false;
    return type == 0;
}

const Block AIR = Block(0, 0, 0, 0);
const Block DIRT_BLOCK = Block(2, 2, 2, 2);
const Block GRASS_BLOCK = Block(1, 4 , 3, 2);
const Block STONE_BLOCK = Block(3, 1, 1, 1);
const Block OAK_LOG = Block(4, 6, 5, 6);
