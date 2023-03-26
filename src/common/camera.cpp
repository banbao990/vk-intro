#include "camera.h"
#include "utils.h"

#include <imgui.h>
#include <glm/gtx/transform.hpp>

ModelViewerCamera::ModelViewerCamera(float field_of_view, float pitch, float yaw, float focus_height, float distance)
    : _focus_height(focus_height), _distance(distance) {
    _pitch = glm::radians(pitch);
    _yaw = glm::radians(yaw);
    _field_of_view = glm::radians(field_of_view);
}

void ModelViewerCamera::draw_ui() {
    ImGui::SliderAngle("Field Of View", &_field_of_view, 0.1f, 180.0f);
    ImGui::SliderAngle("Pitch", &_pitch, 10.0f, 170.0f);
    ImGui::SliderAngle("Yaw", &_yaw);
    ImGui::SliderFloat("Focus Height", &_focus_height, -3.0f, 3.0f);
    ImGui::SliderFloat("Distance", &_distance, 0.5f, 30.0f);
}

glm::vec3 ModelViewerCamera::position() const {
    return _distance * vkutils::polar_to_cartesian(_yaw, _pitch) +
        glm::vec3(0.0f, _focus_height, 0.0f);
}

glm::mat4 ModelViewerCamera::projection(float aspect) const {
    return glm::perspective(_field_of_view, aspect, 0.01f, 100.0f);
}

glm::mat4 ModelViewerCamera::view() const {
    glm::vec3 pos = position();
    glm::mat4 view =
        glm::lookAt(pos, glm::vec3(0, _focus_height, 0), glm::vec3(0, 1, 0));
    return view;
}
