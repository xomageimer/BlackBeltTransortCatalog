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
#include "yellow_pages_manager.h"

#include "distance.h"
#include "responses.h"

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
    struct Datetime {
        size_t day;
        size_t hours;
        size_t minutes;
        double part_of_minute;
    };
    double ToMinute(Datetime const &);
    Datetime ToDatetime(double minutes, size_t day);
    void ExtraTime(YellowPages::Company const &, Datetime const &, double * min_time, double cur_time, double & result);

    int ComputeStopsDistance(const Stop& lhs, const Stop& rhs);
    int ComputeRouteDistance(std::vector<std::string> const & stops, const std::unordered_map<std::string, Stop> &);
    double ComputeTimeToWalking(double meters, double walk_speed);
    double ComputeGeoDistance(std::vector<std::string> const & stops, const std::unordered_map<std::string, Stop> &);
    std::set<std::string> GetBearingPoints(const std::unordered_map<std::string, stop_n_companies> &, const std::unordered_map<std::string, Bus> &);

    using DBItem = std::variant<Stop, Bus>;
    using StopRespType = std::shared_ptr<StopResponse>;
    using BusRespType = std::shared_ptr<BusResponse>;
    using MapRespType = std::shared_ptr<MapResponse>;

    using stop_n_companies = std::variant<Stop, YellowPages::Company>;

    struct DataBase{
    private:
        std::unordered_map<std::string, Bus> pure_buses;
        std::unordered_map<std::string, Stop> pure_stops;

        std::unordered_map<std::string, StopRespType> stops;
        std::unordered_map<std::string, BusRespType> buses;

        std::unique_ptr<DataBaseRouter> router;
        std::unique_ptr<DataBaseSvgBuilder> svg_builder;
        std::unique_ptr<DataBaseYellowPages> yellow_pages_db;
    public:
        explicit DataBase(std::istream & is);
        [[deprecated]] DataBase(const std::vector<DBItem>&, RoutingSettings routing_settings_);
        DataBase(std::vector<DBItem>, RoutingSettings routing_settings_, RenderSettings render_settings);
        DataBase(std::vector<DBItem>, YellowPages::Database, RoutingSettings routing_settings_, RenderSettings render_settings);

        [[nodiscard]] ResponseType FindBus(const std::string & title) const;
        [[nodiscard]] ResponseType FindStop(const std::string & title) const;
        [[nodiscard]] ResponseType FindRoute(const std::string & from, const std::string & to) const;
        [[nodiscard]] ResponseType BuildMap() const;
        [[nodiscard]] ResponseType FindCompanies(const std::vector<std::shared_ptr<Query>> & querys) const;
        [[nodiscard]] ResponseType FindRouteToCompanies(const std::string & from, const Datetime & cur_time, const std::vector<std::shared_ptr<Query>> & querys) const;

        RoutingSettings GetSettings() const {
            return router->GetSettings();
        }
        void Serialize(std::ostream & os) const;
    private:
        void Deserialize(std::istream & is);
        static ResponseType GenerateBad() ;
        void Init(std::vector<DBItem> const &);
    };
}

#endif //DATA_STRUCTURE_H
