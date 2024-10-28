#ifndef BLOCK_H
#define BLOCK_H

#include <cstdint>

class Block {

public:
	uint8_t type;
	uint8_t topTexture;
	uint8_t sideTexture;
	uint8_t bottomTexture;

	Block(uint8_t blockType = 0, uint8_t topTex = 0, uint8_t sideTex = 0, uint8_t bottomTex = 0)
		: type(blockType), topTexture(topTex), sideTexture(sideTex), bottomTexture(bottomTex) {}

	bool isAir() const;
};

extern const Block AIR;
extern const Block DIRT_BLOCK;
extern const Block GRASS_BLOCK;
extern const Block STONE_BLOCK;
extern const Block OAK_LOG;

#endif // BLOCK_H