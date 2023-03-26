#pragma once

#include <glm/glm.hpp>

class ModelViewerCamera {
public:
    ModelViewerCamera(float field_of_view, float pitch, float yaw, float focus_height, float distance);
    void draw_ui();

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    glm::vec3 position() const;

private:
    float _focus_height = 0.25f;
    float _pitch = glm::radians(60.0f);
    float _yaw = glm::radians(60.0f);
    float _field_of_view = glm::radians(25.0f);
    float _distance = 2.5f;
};