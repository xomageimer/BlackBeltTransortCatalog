#ifndef DISTANCE_H
#define DISTANCE_H

struct Distance {
    static inline const double PI = 3.1415926535;
    Distance() : long_m(0), lat_m(0) {};
    Distance(double lo, double la) : long_m(lo), lat_m(la) {}
    Distance(double lo, double la, bool b) : long_m(lo), lat_m(la), is_degrees(b) {}

    Distance(Distance const &) = default;
    Distance& operator=(Distance const &) = default;

    [[nodiscard]] Distance FromDegrees() const {
        if (is_degrees) {
            return {long_m * PI / 180.0, lat_m * PI / 180.0, false};
        }
        return {long_m, lat_m, false};
    }

    [[nodiscard]] double GetLongitude() const {
        return long_m;
    }
    [[nodiscard]] double GetLatitude() const {
        return lat_m;
    }

private:
    double long_m,lat_m;
    mutable bool is_degrees = true;
};

double ComputeDistance(const Distance & lhs, const Distance & rhs);

#endif //DISTANCE_H
