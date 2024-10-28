#include "chunk.h"
#include "vertex.h"
#include "../logger.h"

Chunk::Chunk(glm::vec3 position): position(position) {
	translationMatrix = glm::translate(glm::mat4(1.0f), position);
	scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
	rotationMatrix = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));
	modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;

	chunkCenter = position + glm::vec3(CHUNK_SIZE_X / 2.0f, CHUNK_SIZE_Y / 2.0f, CHUNK_SIZE_Z / 2.0f);
	chunkRadius = glm::sqrt((CHUNK_SIZE_X * CHUNK_SIZE_X) + (CHUNK_SIZE_Y * CHUNK_SIZE_Y) + (CHUNK_SIZE_Z * CHUNK_SIZE_Z)) / 2.0f;

	vertexAndIndexBufferUploaded = false;

	vertexBuffer = VK_NULL_HANDLE;
	vertexBufferMemory = VK_NULL_HANDLE;
	indexBuffer = VK_NULL_HANDLE;
	indexBufferMemory = VK_NULL_HANDLE;
}

void Chunk::setBlock(int x, int y, int z, Block block) {
	blocks[x][y][z] = block;
}

std::optional<Block> Chunk::getBlock(int x, int y, int z) const {
	if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
		return std::nullopt;  // Not in chunk
	}
	return blocks[x][y][z];
}

void Chunk::generateChunkMesh() {
	vertices.clear();
	indices.clear();
	vertices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 24); // 24 vertices per block
	indices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 36);  // 36 indices per block

	for (int x = 0; x < CHUNK_SIZE_X; x++) {
		for (int y = 0; y < CHUNK_SIZE_Y; y++) {
			for (int z = 0; z < CHUNK_SIZE_Z; z++) {

				if (blocks[x][y][z].type != TYPE_AIR) {
					
					loadFacesToMesh(x, y, z);
				}

			}
		}
	}

}

void Chunk::loadFacesToMesh(int x, int y, int z) {
	Block& block = blocks[x][y][z];

	glm::vec3 blockPos = glm::vec3(x, y, z);

	// FRONT face (positive Z)
	if (z == CHUNK_SIZE_Z - 1 || (z + 1 < CHUNK_SIZE_Z && blocks[x][y][z + 1].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::FRONT, block.sideTexture);

	// BACK face (negative Z)
	if (z == 0 || (z - 1 >= 0 && blocks[x][y][z - 1].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::BACK, block.sideTexture);

	// RIGHT face (positive X)
	if (x == CHUNK_SIZE_X - 1 || (x + 1 < CHUNK_SIZE_X && blocks[x + 1][y][z].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::RIGHT, block.sideTexture);

	// LEFT face (negative X)
	if (x == 0 || (x - 1 >= 0 && blocks[x - 1][y][z].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::LEFT, block.sideTexture);

	// TOP face (positive Y)
	if (y == CHUNK_SIZE_Y - 1 || (y + 1 < CHUNK_SIZE_Y && blocks[x][y + 1][z].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::TOP, block.topTexture);

	// BOTTOM face (negative Y)
	if (y == 0 || (y - 1 >= 0 && blocks[x][y - 1][z].type == TYPE_AIR))
		addFaceToMesh(blockPos, FaceDirection::BOTTOM, block.bottomTexture);
}

void Chunk::addFaceToMesh(glm::vec3 position, FaceDirection faceDirection, int texIndex) {
	std::vector<Vertex> faceVertices;

	switch (faceDirection) {
	case FaceDirection::TOP:
		faceVertices = {
			{position + glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec2(0.0f, 1.0f), texIndex},  // Top-left
			{position + glm::vec3(1, 1, 0), glm::vec3(0, 1, 0), glm::vec2(1.0f, 1.0f), texIndex},  // Top-right
			{position + glm::vec3(1, 1, 1), glm::vec3(0, 1, 0), glm::vec2(1.0f, 0.0f), texIndex},  // Bottom-right
			{position + glm::vec3(0, 1, 1), glm::vec3(0, 1, 0), glm::vec2(0.0f, 0.0f), texIndex}   // Bottom-left
		};
		break;

	case FaceDirection::BOTTOM:
		faceVertices = {
			{position + glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0), glm::vec2(1.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 0, 1), glm::vec3(0, -1, 0), glm::vec2(1.0f, 0.0f), texIndex},
			{position + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0), glm::vec2(0.0f, 0.0f), texIndex}
		};
		break;

	case FaceDirection::FRONT:
		faceVertices = {
			{position + glm::vec3(0, 0, 1), glm::vec3(0, 0, 1), glm::vec2(0.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 0, 1), glm::vec3(0, 0, 1), glm::vec2(1.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 1, 1), glm::vec3(0, 0, 1), glm::vec2(1.0f, 0.0f), texIndex},
			{position + glm::vec3(0, 1, 1), glm::vec3(0, 0, 1), glm::vec2(0.0f, 0.0f), texIndex}
		};
		break;

	case FaceDirection::BACK:
		faceVertices = {
			{position + glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec2(0.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec2(1.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 1, 0), glm::vec3(0, 0, -1), glm::vec2(1.0f, 0.0f), texIndex},
			{position + glm::vec3(0, 1, 0), glm::vec3(0, 0, -1), glm::vec2(0.0f, 0.0f), texIndex}
		};
		break;

	case FaceDirection::LEFT:
		faceVertices = {
			{position + glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0), glm::vec2(0.0f, 1.0f), texIndex},
			{position + glm::vec3(0, 0, 1), glm::vec3(-1, 0, 0), glm::vec2(1.0f, 1.0f), texIndex},
			{position + glm::vec3(0, 1, 1), glm::vec3(-1, 0, 0), glm::vec2(1.0f, 0.0f), texIndex},
			{position + glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0), glm::vec2(0.0f, 0.0f), texIndex}
		};
		break;

	case FaceDirection::RIGHT:
		faceVertices = {
			{position + glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec2(0.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 0, 1), glm::vec3(1, 0, 0), glm::vec2(1.0f, 1.0f), texIndex},
			{position + glm::vec3(1, 1, 1), glm::vec3(1, 0, 0), glm::vec2(1.0f, 0.0f), texIndex},
			{position + glm::vec3(1, 1, 0), glm::vec3(1, 0, 0), glm::vec2(0.0f, 0.0f), texIndex}
		};
		break;
	}

	for (const auto& vertex : faceVertices) {
		vertices.push_back(vertex);
	}

	int startIndex = vertices.size() - 4;
	indices.push_back(startIndex + 0);
	indices.push_back(startIndex + 1);
	indices.push_back(startIndex + 2);
	indices.push_back(startIndex + 2);
	indices.push_back(startIndex + 3);
	indices.push_back(startIndex + 0);
}

void Chunk::cleanup() {
	indices.clear();
	vertices.clear();
}