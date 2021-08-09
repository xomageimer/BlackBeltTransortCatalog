#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <string>
#include <variant>

#include <unordered_map>

#include "route_manager.h"
#include "render_manager.h"

#include "distance.h"
#include "responses.h"

template <typename T>
using Dict = std::map<std::string, const T *>;

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
    int ComputeRouteDistance(std::vector<std::string> const & stops, const std::unordered_map<std::string, Stop> &);
    double ComputeGeoDistance(std::vector<std::string> const & stops, const std::unordered_map<std::string, Stop> &);
    std::set<std::string> GetBearingPoints(const std::unordered_map<std::string, Stop> &, const std::unordered_map<std::string, Bus> &);

    using DBItem = std::variant<Stop, Bus>;
    using StopRespType = std::shared_ptr<StopResponse>;
    using BusRespType = std::shared_ptr<BusResponse>;
    using MapRespType = std::shared_ptr<MapResponse>;

    struct DataBase{
    private:
        std::unordered_map<std::string, Bus> pure_buses;
        std::unordered_map<std::string, Stop> pure_stops;

        std::unordered_map<std::string, StopRespType> stops;
        std::unordered_map<std::string, BusRespType> buses;

        std::unique_ptr<DataBaseRouter> router;
        std::unique_ptr<DataBaseSvgBuilder> svg_builder;
    public:
        DataBase(std::istream & is);
        [[deprecated]] DataBase(const std::vector<DBItem>&, std::pair<double, int> routing_settings_);
        DataBase(std::vector<DBItem>, std::pair<double, int> routing_settings_, RenderSettings render_settings);

        ResponseType FindBus(const std::string & title) const;
        ResponseType FindStop(const std::string & title) const;
        ResponseType FindRoute(const std::string & from, const std::string & to) const;
        ResponseType BuildMap() const;

        void Serialize(std::ostream & os) const;
    private:
        void Deserialize(std::istream & is);
        static ResponseType GenerateBad() ;
        void Init(std::vector<DBItem> const &);
    };
}

#endif //DATA_STRUCTURE_H
