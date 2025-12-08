#include <libDomain/Compute.h>

#include <numbers>
#include <cmath>

namespace compute 
{


// Based in approximation shared here: https://stackoverflow.com/a/27126820

double toRad(double degree)
{
    return degree/180 * std::numbers::pi;
}

double calculateDistance(glm::dvec2 coord1, glm::dvec2 coord2)
{
    double dist;
    dist = sin(toRad(coord1.x)) * sin(toRad(coord2.x)) + cos(toRad(coord1.x)) * cos(toRad(coord2.x)) * cos(toRad(coord1.y - coord2.y));
    dist = acos(dist);
    dist = earthRadiusKm * dist;
    return dist;
}

}