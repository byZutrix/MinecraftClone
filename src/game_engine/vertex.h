#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    int texIndex;
};

#endif // VERTEX_H