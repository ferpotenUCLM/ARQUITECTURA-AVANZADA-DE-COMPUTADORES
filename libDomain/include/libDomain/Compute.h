#pragma once

#include <glm/glm.hpp>

namespace compute
{
    constexpr float earthRadiusKm = 6371.;

    double toRad(double degree);
    double calculateDistance(glm::dvec2 coord1, glm::dvec2 coord2);
}