#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <string>
#include <variant>

#include <unordered_map>

#include "route_structure.h"
#include "distance.h"

#include "responses.h"

template <typename T>
using Dict = std::unordered_map<std::string, const T *>;

namespace Data_Structure {
    struct Bus {
        std::string name;
        std::vector<std::string> stops;
        bool is_roundtrip;
    };
    struct Stop {
        std::string name;
        Distance dist;
        std::map<std::string, int> adjacent_stops;
    };
    int ComputeStopsDistance(const Stop& lhs, const Stop& rhs);
    int ComputeRouteDistance(std::vector<std::string> const & stops, Dict<Stop> const &);
    double ComputeGeoDistance(std::vector<std::string> const & stops, Dict<Stop> const &);

    using DBItem = std::variant<Stop, Bus>;
    using StopRespType = std::shared_ptr<StopResponse>;
    using BusRespType = std::shared_ptr<BusResponse>;

    struct DataBase{
    private:
        std::unordered_map<std::string, StopRespType> stops;
        std::unordered_map<std::string, BusRespType> buses;

        std::unique_ptr<DataBaseRouter> router;
    public:
        DataBase(std::vector<DBItem>, std::pair<double, int> routing_settings_);

        ResponseType FindBus(const std::string & title) const;
        ResponseType FindStop(const std::string & title) const;
        ResponseType FindRoute(const std::string & from, const std::string & to) const;
    private:
        static ResponseType GenerateBad() ;
    };
}

#endif //DATA_STRUCTURE_H
