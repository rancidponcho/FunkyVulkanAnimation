#pragma once

#include "sve_model.hpp"

// std
#include <memory>

namespace sve {

struct Transform2dComponent {
    glm::vec2 translation{};  // position offset
    glm::vec2 scale{1.f, 1.f};
    float rotation;

    glm::mat2 mat2() {
        // rotation matrix from angle
        const float s = glm::sin(rotation);  //  Rotation matrix
        const float c = glm::cos(rotation);  // { Cosine, Sine   }
        glm::mat2 rotMat{{c, s}, {-s, c}};   // {  -Sine, Cosine }
        // scale matrix
        glm::mat2 scaleMat{{scale.x, .0f}, {.0f, scale.y}};
        return rotMat * scaleMat;
    }
};

// eventually use entity component system
class SveGameObject {
   public:
    using id_t = unsigned int;

    static SveGameObject createGameObject() {
        static id_t currentId = 0;
        return SveGameObject(currentId++);
    }

    SveGameObject(const SveGameObject &) = delete;
    SveGameObject &operator=(const SveGameObject &) = delete;
    SveGameObject(SveGameObject &&) = default;
    SveGameObject &operator=(SveGameObject &&) = default;

    id_t getId() { return id; }

    std::shared_ptr<SveModel> model;  // shared model among all game objects
    glm::vec3 color{};
    Transform2dComponent transform2d{};

   private:
    SveGameObject(id_t objId) : id(objId) {}  // private constructor to force use of factory method

    id_t id;  // unique id
};

}  // namespace sve