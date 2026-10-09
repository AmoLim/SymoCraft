#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/component.h>
#include <symocraft/ecs/registry.h>
#include <algorithm>

namespace SymoCraft {
    Camera::Camera(ECS::Registry& registry, float, float, glm::vec3 position)
        : registry_(registry)
    {
        entity_id = registry_.CreateEntity();
        registry_.AddComponent<Transform>(entity_id);
        auto& transform = registry_.GetComponent<Transform>(entity_id);
        transform = {};
        transform.scale = glm::vec3(1.0f);
        transform.yaw = -90.0f;
        transform.position = position;
    }

    void Camera::Scroll(double y_offset)
    {
        fov_ = std::clamp(fov_ - static_cast<float>(y_offset), 1.0f, 45.0f);
    }

    glm::mat4 Camera::GetCameraViewMat() const
    {
        const auto& transform = registry_.GetComponent<Transform>(entity_id);
        return glm::lookAt(transform.position, transform.position + transform.front, transform.up);
    }

    glm::mat4 Camera::GetCameraProjMat(float aspect_ratio) const
    {
        return glm::perspective(glm::radians(fov_), aspect_ratio, 0.1f, 2000.0f);
    }

    glm::vec3 Camera::GetCameraPos() const { return registry_.GetComponent<Transform>(entity_id).position; }
    glm::vec2 Camera::GetCameraPos_vec2() const
    {
        const auto position = GetCameraPos();
        return {position.x, position.z};
    }
    void Camera::SetCameraPos(const glm::vec3& position) { registry_.GetComponent<Transform>(entity_id).position = position; }
    float Camera::GetYaw() const { return registry_.GetComponent<Transform>(entity_id).yaw; }
    void Camera::SetYaw(float yaw) { registry_.GetComponent<Transform>(entity_id).yaw = yaw; }
    float Camera::GetPitch() const { return registry_.GetComponent<Transform>(entity_id).pitch; }
    void Camera::SetPitch(float pitch) { registry_.GetComponent<Transform>(entity_id).pitch = pitch; }
    float Camera::GetFov() const { return fov_; }
    glm::vec3 Camera::GetCameraFront() const { return registry_.GetComponent<Transform>(entity_id).front; }
    glm::vec3 Camera::GetCameraUp() const { return registry_.GetComponent<Transform>(entity_id).up; }
}
