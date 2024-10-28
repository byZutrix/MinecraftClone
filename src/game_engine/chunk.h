#ifndef CHUNK_H
#define CHUNK_H

#include "vertex.h"
#include "block.h"
#include <optional>

// const chunk constants
const int CHUNK_SIZE_X = 16;
const int CHUNK_SIZE_Y = 64;
const int CHUNK_SIZE_Z = 16;

class Chunk {
public:

	//Chunk();
	Chunk(glm::vec3 position);

	enum BlockType {
		TYPE_AIR = 0,
		TYPE_GRASS_BLOCK = 1,
		TYPE_DIRT = 2,
		TYPE_STONE = 3,
		TYPE_OAK_LOG = 4
	};

	enum FaceTypes {
		FACE_STONE = 1,
		FACE_DIRT = 2,
		FACE_GRASS_BLOCK_SIDE = 3,
		FACE_GRASS_BLOCK_TOP = 4,
		FACE_OAK_LOG = 5,
		FACE_OAK_LOG_TOP = 6
	};

	enum FaceDirection {
		FRONT = 0,
		BACK = 1,
		LEFT = 2,
		RIGHT = 3,
		TOP = 4,
		BOTTOM = 5,
	};

	struct DeferredFace {
		glm::vec3 position;
		FaceDirection direction;
		int textureIndex;
	};

	glm::vec3 position;
	glm::mat4 translationMatrix;
	glm::mat4 scaleMatrix;
	glm::mat4 rotationMatrix;
	glm::mat4 modelMatrix;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	bool vertexAndIndexBufferUploaded;

	glm::vec3 chunkCenter;
	float chunkRadius;

	void generateChunkMesh();

	void setBlock(int x, int y, int z, Block block);
	std::optional<Block> getBlock(int x, int y, int z) const;

	const std::vector<Vertex>& getVertices() const { return vertices; }
	const std::vector<uint32_t>& getIndices() const { return indices; }

	void cleanup();

	void addFaceToMesh(glm::vec3 position, FaceDirection faceDirection, int texIndex);

	std::vector<DeferredFace> deferredFaces;

private:
	Block blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	void loadFacesToMesh(int x, int y, int z);
};

#endif // CHUNK_H