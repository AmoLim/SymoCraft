#pragma once

#include <symocraft/foundation/math.h>
#include <symocraft/ecs/registry.h>

namespace SymoCraft {
    namespace ECS { class Registry; }

    // Registry owns the camera entity and must outlive this borrowed facade.
    class Camera {
    public:
        Camera(ECS::Registry& registry, float width, float height, glm::vec3 position = glm::vec3(0.0f));
        ECS::EntityId entity_id;
        void Scroll(double y_offset);
        glm::mat4 GetCameraViewMat() const;
        glm::mat4 GetCameraProjMat(float aspect_ratio) const;
        glm::vec3 GetCameraPos() const;
        glm::vec2 GetCameraPos_vec2() const;
        void SetCameraPos(const glm::vec3& position);
        float GetYaw() const;
        void SetYaw(float yaw);
        float GetPitch() const;
        void SetPitch(float pitch);
        float GetFov() const;
        glm::vec3 GetCameraFront() const;
        glm::vec3 GetCameraUp() const;
    private:
        ECS::Registry& registry_;
        float fov_ = 45.0f;
    };
}
