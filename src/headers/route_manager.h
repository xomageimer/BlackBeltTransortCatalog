#ifndef ROUTE_STRUCTURE_H
#define ROUTE_STRUCTURE_H

#include <unordered_map>
#include <utility>

#include "graph.h"
#include "router.h"

#include "responses.h"

namespace Serialize {
    struct TransportCatalog;
    struct Router;
}

template <typename T>
using Dict = std::map<std::string, const T *>;

namespace Data_Structure {
    using RouteRespType = std::shared_ptr<RouteResponse>;
    struct DataBaseRouter {
    private:
        struct RoutingSettings {
            explicit RoutingSettings(std::pair<double, int> pr) : bus_wait_time(pr.first), bus_velocity(pr.second) {}

            double bus_wait_time{};
            int bus_velocity{};
        };
        const RoutingSettings routing_settings;
        Graph::DirectedWeightedGraph<double> graph_map;
        std::shared_ptr<Graph::Router<double>> router;

        std::unordered_map<Graph::EdgeID, RouteResponse::ItemPtr> edge_by_bus;
        struct vertices_path {
            Graph::VertexID inp;
            Graph::VertexID out;
        };
        std::unordered_map<std::string, vertices_path> waiting_stops;
        std::vector<std::string> vertices_by_stop;
    public:
        struct proxy_route {
        private:
            std::shared_ptr<Graph::Router<double>> main_router;
            std::optional<Graph::Router<double>::RouteInfo> rf;
        public:
            proxy_route(std::shared_ptr<Graph::Router<double>> router, std::optional<Graph::Router<double>::RouteInfo> rinf) : main_router(std::move(router)),
                                                                                                rf(rinf) {}
            proxy_route(const proxy_route &) = delete;
            proxy_route &operator=(const proxy_route &) = delete;
            proxy_route(proxy_route &&) = default;

            [[nodiscard]] auto GetRoute() const;
            [[nodiscard]] std::optional<Graph::Router<double>::RouteInfo> const & GetInfo() const;
            [[nodiscard]] bool IsValid() const;

            ~proxy_route();
        };

        DataBaseRouter(const Dict<struct Stop>& stops, const Dict<struct Bus>& buses, std::pair<double, int> routing_settings_);
        explicit DataBaseRouter(Serialize::Router const & router_mes);

        RouteRespType CreateRoute(std::string const & from, std::string const & to);
        void Serialize(Serialize::TransportCatalog &) const;
    private:
        void FillGraphWithStops(const Dict<struct Stop>&);
        void FillGraphWithBuses(const Dict<struct Bus>&, const Dict<Data_Structure::Stop> &);
    };
}

#endif //ROUTE_STRUCTURE_H
