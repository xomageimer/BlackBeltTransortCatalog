#include "distance.h"

#include <cmath>

const double EarthRadius = 6'371'000;

double ComputeDistance(const Distance &lhs, const Distance &rhs) {
    auto lhs_ = lhs.FromDegrees();
    auto rhs_ = rhs.FromDegrees();
    return (std::acos(std::sin(lhs_.GetLatitude()) * std::sin(rhs_.GetLatitude()) +
                      std::cos(lhs_.GetLatitude()) * cos(rhs_.GetLatitude()) * cos(std::abs(lhs_.GetLongitude() - rhs_.GetLongitude()))) * EarthRadius);
}