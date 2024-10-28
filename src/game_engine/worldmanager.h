#ifndef WORLDMANAGER_H
#define WORLDMANAGER_H

#include <utility>
#include <functional>
#include <unordered_map>
#include <memory>
#include <queue>
#include "chunk.h"
#include "../FastNoiseLite.h"
#include <optional>

struct pair_hash {
	template <class T1, class T2>
	std::size_t operator () (const std::pair<T1, T2>& pair) const {
		auto hash1 = std::hash<T1>{}(pair.first);
		auto hash2 = std::hash<T2>{}(pair.second);
		return hash1 ^ hash2;
	}
};

class WorldManager {
public:
	WorldManager();

	void generateChunksAround(glm::vec3 cameraPos, int viewDistance);
	void unloadDistantChunks(glm::vec3 cameraPos, int viewDistance, VkDevice device);

	void processChunkQueue(int chunksPerFrame);

	std::shared_ptr<Chunk> getChunk(int x, int z);

	void setBlock(int x, int y, int z, Block block);
	std::optional<Block> getBlockInChunk(int x, int y, int z, Chunk* chunk);

	const std::unordered_map<std::pair<int, int>, std::shared_ptr<Chunk>, pair_hash> getChunks(){ return chunks; }

	void cleanupBuffers(VkDevice device, Chunk* chunk);

	void clearChunks();

private:

	struct ChunkPriority {
		int x, z;
		float distance;

		bool operator<(const ChunkPriority& other) const {
			return distance > other.distance;  // Closer chunks have higher priority
		}
	};

	// FAST NOISE LITE
	FastNoiseLite noiseGenerator;
	float getHeight(float x, float z);
	void generateChunks(int chunkX, int chunkZ);

	// WORLD DATA STUFF
	std::pair<int, int> getChunkCoordinates(glm::vec3 cameraPos);

	std::unordered_map<std::pair<int, int>, std::shared_ptr<Chunk>, pair_hash> chunks;

	std::priority_queue<ChunkPriority> chunkLoadingPriorityQueue;

	void generateChunkMesh(int chunkX, int chunkZ, Chunk* chunk);
	void processDeferredFaces(int chunkX, int chunkZ);
};

#endif // !WORLDMANAGER_H
