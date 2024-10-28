#include "worldmanager.h"
#include "../logger.h"

const float groundLevel = 0.0f;
const float maxHeight = 50.0f;
const float noiseScale = 0.1f;
WorldManager::WorldManager() {
	noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin); // PERLIN NOISE
	noiseGenerator.SetFrequency(noiseScale); // CHANGEABLE
}

float WorldManager::getHeight(float x, float z) {
	float noiseValue = noiseGenerator.GetNoise(x * noiseScale, z * noiseScale);

	return (noiseValue + 1.0f) / 2.0f * maxHeight;
}

void WorldManager::generateChunks(int chunkX, int chunkZ) {
	glm::vec3 chunkPosition = glm::vec3(chunkX * CHUNK_SIZE_X, 0, chunkZ * CHUNK_SIZE_Z);

	Chunk* chunk = new Chunk(chunkPosition);

	for (int x = 0; x < CHUNK_SIZE_X; x++) {
		for (int z = 0; z < CHUNK_SIZE_Z; z++) {

			int worldX = chunkX * CHUNK_SIZE_X + x;
			int worldZ = chunkZ * CHUNK_SIZE_Z + z;

			float height = groundLevel + getHeight(worldX, worldZ);
			int highestBlockY = static_cast<int>(height);

			for (int y = 0; y < CHUNK_SIZE_Y; y++) {

				Block block;

				if (y < highestBlockY - 5) {
					block = STONE_BLOCK; // Deep stone
				}
				else if (y < highestBlockY) {
					block = DIRT_BLOCK; // Dirt layer below grass
				}
				else if (y == highestBlockY) {
					block = GRASS_BLOCK; // Grass on top
				}
				else {
					continue; // Skip air blocks for culling
				}

				chunk->setBlock(x, y, z, block);
				
				//glm::vec3 position = glm::vec3(x, y, z);
				/*if (z == CHUNK_SIZE_Z - 1 || chunk->getBlock(x, y, z + 1).isAir()) {
					chunk->addFaceToMesh(position, Chunk::FaceDirection::FRONT, block.sideTexture);
				}
				if (z == 0 || chunk->getBlock(x, y, z - 1).isAir()) {
					chunk->addFaceToMesh(position, Chunk::FaceDirection::BACK, block.sideTexture);
				}
				if (x == CHUNK_SIZE_X - 1 || chunk->getBlock(x + 1, y, z).isAir()) {
					chunk->addFaceToMesh(position, Chunk::FaceDirection::RIGHT, block.sideTexture);
				}
				if (x == 0 || chunk->getBlock(x - 1, y, z).isAir()) {
					chunk->addFaceToMesh(position, Chunk::FaceDirection::LEFT, block.sideTexture);
				}*/
				//if (y == CHUNK_SIZE_Y - 1 || chunk->getBlock(x, y + 1, z).isAir()) {
				//	chunk->addFaceToMesh(position, Chunk::FaceDirection::TOP, block.topTexture);
				//}
				//if (y == 0 || chunk->getBlock(x, y - 1, z).isAir()) {
				//	chunk->addFaceToMesh(position, Chunk::FaceDirection::BOTTOM, block.bottomTexture);
				//}

			}

		}
	}
	generateChunkMesh(chunkX, chunkZ, chunk);
	chunks[{chunkX, chunkZ}] = std::unique_ptr<Chunk>(chunk);
}

void WorldManager::generateChunkMesh(int chunkX, int chunkZ, Chunk* chunk) {

	for (int x = 0; x < CHUNK_SIZE_X; x++) {
		for (int z = 0; z < CHUNK_SIZE_Z; z++) {
			for (int y = 0; y < CHUNK_SIZE_Y; y++) {

				//int worldX = chunkX * CHUNK_SIZE_X + x;
				//int worldZ = chunkZ * CHUNK_SIZE_Z + z;

				std::optional<Block> blockOpt = getBlockInChunk(x, y, z, chunk);
				if (blockOpt.has_value() && blockOpt->type != 0) {
					Block block = blockOpt.value();
					glm::vec3 position(x, y, z);

					// TOP FACE
					std::optional<Block> blockAboveOpt = chunk->getBlock(x, y + 1, z);
					bool isBlockAboveAir = !blockAboveOpt.has_value() || blockAboveOpt->type == 0;
					if(isBlockAboveAir)
						chunk->addFaceToMesh(position, Chunk::FaceDirection::TOP, block.topTexture);
					
					// BOTTOM FACE
					if (y > 0) {
						std::optional<Block> blockBelowOpt = chunk->getBlock(x, y - 1, z);
						bool isBlockBelowAir = !blockBelowOpt.has_value() || blockBelowOpt->type == 0;
						if (isBlockBelowAir)
							chunk->addFaceToMesh(position, Chunk::FaceDirection::BOTTOM, block.bottomTexture);
					}


					// RIGHT FACE
					if (x == CHUNK_SIZE_X - 1) {
						if (auto rightNeighbor = getChunk(chunkX + 1, chunkZ)) {
							auto rightBlockOpt = rightNeighbor->getBlock(0, y, z);
							if (!rightBlockOpt || rightBlockOpt->type == 0)
								chunk->addFaceToMesh(position, Chunk::FaceDirection::RIGHT, block.sideTexture);
						}
						else {
							// Defer the face if no neighbor
							chunk->deferredFaces.push_back({ position, Chunk::FaceDirection::RIGHT, block.sideTexture });
						}
					}
					else {
						std::optional<Block> blockRightOpt = getBlockInChunk(x + 1, y, z, chunk);
						bool isBlockRightAir = !blockRightOpt.has_value() || blockRightOpt->type == 0;
						if (isBlockRightAir)
							chunk->addFaceToMesh(position, Chunk::FaceDirection::RIGHT, block.sideTexture);
					}


					// LEFT FACE
					if (x == 0) {
						if (auto leftNeighbor = getChunk(chunkX - 1, chunkZ)) {
							auto leftBlockOpt = leftNeighbor->getBlock(CHUNK_SIZE_X - 1, y, z);
							if (!leftBlockOpt || leftBlockOpt->type == 0)
								chunk->addFaceToMesh(position, Chunk::FaceDirection::LEFT, block.sideTexture);
						}
						else {
							// Defer the face if no neighbor
							chunk->deferredFaces.push_back({ position, Chunk::FaceDirection::LEFT, block.sideTexture });
						}
					}
					else {
						std::optional<Block> blockLeftOpt = getBlockInChunk(x - 1, y, z, chunk);
						bool isBlockLeftAir = !blockLeftOpt.has_value() || blockLeftOpt->type == 0;
						if (isBlockLeftAir)
							chunk->addFaceToMesh(position, Chunk::FaceDirection::LEFT, block.sideTexture);
					}

					// FRONT FACE
					if (z == CHUNK_SIZE_Z - 1) {
						if (auto frontNeighbor = getChunk(chunkX, chunkZ + 1)) {
							auto frontBlockOpt = frontNeighbor->getBlock(x, y, 0);
							if (!frontBlockOpt || frontBlockOpt->type == 0) {
								// If there is no front block or it is air, add the front face
								chunk->addFaceToMesh(position, Chunk::FaceDirection::FRONT, block.sideTexture);
							}
						}
						else {
							// Defer the face if no neighbor chunk
							chunk->deferredFaces.push_back({ position, Chunk::FaceDirection::FRONT, block.sideTexture });
						}
					}
					else {
						std::optional<Block> blockFrontOpt = getBlockInChunk(x, y, z + 1, chunk);
						bool isBlockFrontAir = !blockFrontOpt.has_value() || blockFrontOpt->type == 0;
						if (isBlockFrontAir) {
							chunk->addFaceToMesh(position, Chunk::FaceDirection::FRONT, block.sideTexture);
						}
					}


					// BACK FACE
					if (z == 0) {
						if (auto backNeighbor = getChunk(chunkX, chunkZ - 1)) {
							auto backBlockOpt = backNeighbor->getBlock(x, y, CHUNK_SIZE_Z - 1);
							if (!backBlockOpt || backBlockOpt->type == 0) {
								// If there is no back block or it is air, add the back face
								chunk->addFaceToMesh(position, Chunk::FaceDirection::BACK, block.sideTexture);
							}
						}
						else {
							// Defer the face if no neighbor chunk
							chunk->deferredFaces.push_back({ position, Chunk::FaceDirection::BACK, block.sideTexture });
						}
					}
					else {
						std::optional<Block> blockBackOpt = getBlockInChunk(x, y, z - 1, chunk);
						bool isBlockBackAir = !blockBackOpt.has_value() || blockBackOpt->type == 0;
						if (isBlockBackAir) {
							chunk->addFaceToMesh(position, Chunk::FaceDirection::BACK, block.sideTexture);
						}
					}

					// RIGHT FACE
					/*std::optional<Block> blockRightOpt = getBlockInChunk(x + 1, y, z, chunk);
					bool isBlockRightAir = !blockRightOpt.has_value() || blockRightOpt->type == 0;
					if(isBlockRightAir)
						chunk->addFaceToMesh(position, Chunk::FaceDirection::RIGHT, block.sideTexture);

					// LEFT FACE
					std::optional<Block> blockLeftOpt = getBlockInChunk(x - 1, y, z, chunk);
					bool isBlockLeftAir = !blockLeftOpt.has_value() || blockLeftOpt->type == 0;
					if (isBlockLeftAir)
						chunk->addFaceToMesh(position, Chunk::FaceDirection::LEFT, block.sideTexture);

					// FRONT FACE
					std::optional<Block> blockFrontOpt = getBlockInChunk(x, y, z + 1, chunk);
					bool isBlockFrontAir = !blockFrontOpt.has_value() || blockFrontOpt->type == 0;
					if (isBlockFrontAir)
						chunk->addFaceToMesh(position, Chunk::FaceDirection::FRONT, block.sideTexture);

					// BACK FACE
					std::optional<Block> blockBackOpt = getBlockInChunk(x, y, z - 1, chunk);
					bool isBlockBackAir = !blockBackOpt.has_value() || blockBackOpt->type == 0;
					if (isBlockBackAir)
						chunk->addFaceToMesh(position, Chunk::FaceDirection::BACK, block.sideTexture);
						*/
				}

			}
		}
	}
}

void WorldManager::processDeferredFaces(int chunkX, int chunkZ) {
	auto chunk = getChunk(chunkX, chunkZ);
	if (!chunk) return;

	for (auto it = chunk->deferredFaces.begin(); it != chunk->deferredFaces.end();) {
		const auto& face = *it;
		bool isVisible = false;

		switch (face.direction) {
		case Chunk::FaceDirection::LEFT:
			if (auto leftNeighbor = getChunk(chunkX - 1, chunkZ)) {
				auto blockOpt = leftNeighbor->getBlock(CHUNK_SIZE_X - 1, face.position.y, face.position.z);
				isVisible = !blockOpt || blockOpt->type == 0;
			}
			break;
		case Chunk::FaceDirection::RIGHT:
			if (auto rightNeighbor = getChunk(chunkX + 1, chunkZ)) {
				auto blockOpt = rightNeighbor->getBlock(0, face.position.y, face.position.z);
				isVisible = !blockOpt || blockOpt->type == 0;
			}
			break;
		case Chunk::FaceDirection::FRONT:
			if (auto backNeighbor = getChunk(chunkX, chunkZ + 1)) {
				auto blockOpt = backNeighbor->getBlock(face.position.x, face.position.y, 0);
				isVisible = !blockOpt || blockOpt->type == 0;
			}
			break;
		case Chunk::FaceDirection::BACK:
			if (auto frontNeighbor = getChunk(chunkX, chunkZ - 1)) {
				auto blockOpt = frontNeighbor->getBlock(face.position.x, face.position.y, CHUNK_SIZE_Z - 1);
				isVisible = !blockOpt || blockOpt->type == 0;
			}
			break;
		}

		if (isVisible) {
			chunk->addFaceToMesh(face.position, face.direction, face.textureIndex);
			it = chunk->deferredFaces.erase(it);
			chunk->vertexAndIndexBufferUploaded = false;
		}
		else {
			++it;
		}
	}
}


std::pair<int, int> WorldManager::getChunkCoordinates(glm::vec3 cameraPos) {
	int chunkX = static_cast<int>(floor(cameraPos.x / CHUNK_SIZE_X));
	int chunkZ = static_cast<int>(floor(cameraPos.z / CHUNK_SIZE_X));
	return { chunkX, chunkZ };
}

void WorldManager::generateChunksAround(glm::vec3 cameraPos, int viewDistance) {
	int chunkViewDistance = viewDistance;

	std::pair<int, int> chunkCoords = getChunkCoordinates(cameraPos);
	int playerChunkX = chunkCoords.first;
	int playerChunkZ = chunkCoords.second;
	for (int x = playerChunkX - chunkViewDistance; x <= playerChunkX + chunkViewDistance; ++x) {
		for (int z = playerChunkZ - chunkViewDistance; z <= playerChunkZ + chunkViewDistance; ++z) {
			if (!getChunk(x, z)) {
				float dist = glm::length(glm::vec2(x - playerChunkX, z - playerChunkZ));
				chunkLoadingPriorityQueue.push({ x, z, dist });
			}
		}
	}
}

void WorldManager::processChunkQueue(int chunksPerFrame) {
	int chunksProcessed = 0;

	while (!chunkLoadingPriorityQueue.empty() && chunksProcessed < chunksPerFrame) {
		auto chunkPriority = chunkLoadingPriorityQueue.top();
		chunkLoadingPriorityQueue.pop();

		int x = chunkPriority.x;
		int z = chunkPriority.z;

		if (!getChunk(x, z)) {
			generateChunks(x, z);
			chunksProcessed++;

			processDeferredFaces(x - 1, z);
			processDeferredFaces(x + 1, z);
			processDeferredFaces(x, z + 1);
			processDeferredFaces(x, z - 1);
		}
	}
}

std::shared_ptr<Chunk> WorldManager::getChunk(int x, int z) {
	auto chunkCoord = std::make_pair(x, z);
	if (chunks.find(chunkCoord) != chunks.end()) {
		return chunks[chunkCoord];
	}
	return nullptr;
}

void WorldManager::setBlock(int x, int y, int z, Block block) {
	std::pair<int, int> chunkCoords = getChunkCoordinates(glm::vec3(x, y, z));
	int chunkX = chunkCoords.first;
	int chunkZ = chunkCoords.second;
    std::shared_ptr<Chunk> chunk = getChunk(chunkX, chunkZ);
    if (chunk) {
        //int localX = x % CHUNK_SIZE_X;
        //int localY = y;
        //int localZ = z % CHUNK_SIZE_Z;
		chunk->setBlock(x, y, z, block);
    }
}

std::optional<Block> WorldManager::getBlockInChunk(int x, int y, int z, Chunk* chunk) {

	int localX = x % CHUNK_SIZE_X;
	int localZ = z % CHUNK_SIZE_Z;

	if (!chunk) {
		return std::nullopt;
	}
	return chunk->getBlock(localX, y, localZ);
}

// Unload distant chunks
void WorldManager::unloadDistantChunks(glm::vec3 cameraPos, int viewDistance, VkDevice device) {
    int chunkViewDistance = viewDistance * 2; // CHANGE THIS TO BIGGER VALUE LATER
	std::pair<int, int> chunkCoords = getChunkCoordinates(cameraPos);
	int playerChunkX = chunkCoords.first;
	int playerChunkZ = chunkCoords.second;

    for (auto it = chunks.begin(); it != chunks.end(); ) {
        int chunkX = it->first.first;
        int chunkZ = it->first.second;

        if (abs(chunkX - playerChunkX) > chunkViewDistance || abs(chunkZ - playerChunkZ) > chunkViewDistance) {
			vkDeviceWaitIdle(device);
			cleanupBuffers(device, it->second.get());
			it->second.get()->cleanup();
			it = chunks.erase(it);
        }
        else {
            ++it;
        }
    }
}

void WorldManager::cleanupBuffers(VkDevice device, Chunk* chunk) {
	if (chunk->vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, chunk->vertexBuffer, nullptr);
		chunk->vertexBuffer = VK_NULL_HANDLE;
	}

	if (chunk->indexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, chunk->indexBuffer, nullptr);
		chunk->indexBuffer = VK_NULL_HANDLE;
	}

	if (chunk->vertexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, chunk->vertexBufferMemory, nullptr);
		chunk->vertexBufferMemory = VK_NULL_HANDLE;
	}

	if (chunk->indexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, chunk->indexBufferMemory, nullptr);
		chunk->indexBufferMemory = VK_NULL_HANDLE;
	}
}

void WorldManager::clearChunks() {
	chunks.clear();
}