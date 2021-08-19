#include "route_manager.h"
#include "data_manager.h"

#include "transport_catalog.pb.h"

auto Data_Structure::DataBaseRouter::proxy_route::GetRoute() const {
    if (rf)
        return main_router->GetRouteRangeOfEdges(rf->id);
    throw std::logic_error("bad route info value!");
}

Data_Structure::DataBaseRouter::proxy_route::~proxy_route() {
    if (rf)
        main_router->ReleaseRoute(rf->id);
}

std::optional<Graph::Router<double>::RouteInfo> const &Data_Structure::DataBaseRouter::proxy_route::GetInfo() const {
    return rf;
}

bool Data_Structure::DataBaseRouter::proxy_route::IsValid() const {
    if (rf)
        return main_router->CheckCache(rf->id);
    return false;
}

Data_Structure::DataBaseRouter::DataBaseRouter(RouterProto::Router const & router_mes) : routing_settings({router_mes.routing_settings().bus_wait_time(), router_mes.routing_settings().bus_velocity()}),  graph_map(router_mes) {
    for (auto & vert : router_mes.vertexes()){
        waiting_stops.emplace(vert.name(), vertices_path{vert.route_data_in(0).vertex_id(), vert.route_data_out(0).vertex_id()});
    }

    for (auto & edge : router_mes.edges()){
        RouteResponse::ItemPtr item;
        if (edge.has_count()) {
            item = std::make_shared<RouteResponse::Bus>(edge.count().count());
        } else {
            item = std::make_shared<RouteResponse::Wait>();
        }

        item->name = edge.name();
        item->time = edge.weight();
        edge_by_bus.emplace(edge.id(), std::move(item));
    }

    router = std::make_shared<Graph::Router<double>>(graph_map, router_mes);
}


Data_Structure::DataBaseRouter::DataBaseRouter(const std::unordered_map<std::string, Stop> & stops, const std::unordered_map<std::string, Bus> & buses,
                                               RoutingSettings routing_settings_) : graph_map(stops.size() * 2), routing_settings(routing_settings_) {
    FillGraphWithStops(stops);
    FillGraphWithBuses(buses, stops);

    router = std::make_shared<Graph::Router<double>>(graph_map);
}

void Data_Structure::DataBaseRouter::FillGraphWithStops(const std::unordered_map<std::string, Stop> & stops) {
    Graph::VertexID vertex_id = 0;
    for (auto & [title, _] : stops){
        vertices_path & vert_ids = waiting_stops[title];
        vert_ids.inp = vertex_id++;
        vert_ids.out = vertex_id++;

        auto edge_id = graph_map.AddEdge({
            vert_ids.inp,
            vert_ids.out,
            routing_settings.bus_wait_time
        });
        auto cur_item = std::make_shared<RouteResponse::Wait>();
        cur_item->time = routing_settings.bus_wait_time;
        cur_item->name = title;
        edge_by_bus.emplace(std::piecewise_construct, std::forward_as_tuple(edge_id), std::forward_as_tuple(std::move(cur_item)));
    }
}

void Data_Structure::DataBaseRouter::FillGraphWithBuses(const std::unordered_map<std::string, Bus> & buses, const std::unordered_map<std::string, Stop> & stops) {
    for (auto & [_, bus] : buses){
        size_t stop_count = bus.stops.size();
        if (stop_count <= 1)
            continue;

        auto range = !bus.is_roundtrip
                     ? Ranges::ToMiddle(Ranges::AsRange(bus.stops))
                     : Ranges::AsRange(bus.stops);
        for (auto bus_stop = range.begin(); bus_stop != range.end(); bus_stop++){
            int total_distance = 0;
            auto & cur_stop = *bus_stop;
            size_t i = 0;
            for (auto stop_it = bus_stop; stop_it != std::prev(range.end()); stop_it++)
            {
                auto time = Data_Structure::ComputeStopsDistance(stops.at(*stop_it),
                                                                 stops.at(*std::next(stop_it)));
                total_distance += time;
                auto edge_id = graph_map.AddEdge(
                        {waiting_stops.at(cur_stop).out, waiting_stops.at(*std::next(stop_it)).inp,
                         (static_cast<double>(total_distance) / (routing_settings.bus_velocity / 3.6)) / 60});
                auto cur_item = std::make_shared<RouteResponse::Bus>(++i);
                cur_item->time = (static_cast<double>(total_distance) / (routing_settings.bus_velocity / 3.6)) / 60;
                cur_item->name = bus.name;
                edge_by_bus.emplace(std::piecewise_construct, std::forward_as_tuple(edge_id),
                                    std::forward_as_tuple(std::move(cur_item)));
            }
            total_distance = 0;
            i = 0;
            if (!bus.is_roundtrip) {
                for (auto stop_it = bus_stop; stop_it != range.begin(); stop_it--)
                {
                    auto time = Data_Structure::ComputeStopsDistance(stops.at(*stop_it),
                                                                     stops.at(*std::prev(stop_it)));
                    total_distance += time;
                    auto edge_id = graph_map.AddEdge(
                            {waiting_stops.at(cur_stop).out, waiting_stops.at(*std::prev(stop_it)).inp,
                             (static_cast<double>(total_distance) / (routing_settings.bus_velocity / 3.6)) / 60});
                    auto cur_item = std::make_shared<RouteResponse::Bus>(++i);
                    cur_item->time = (static_cast<double>(total_distance) / (routing_settings.bus_velocity / 3.6)) / 60;
                    cur_item->name = bus.name;
                    edge_by_bus.emplace(std::piecewise_construct, std::forward_as_tuple(edge_id),
                                        std::forward_as_tuple(std::move(cur_item)));
                }
            }
        }
    }
}

Data_Structure::RouteRespType Data_Structure::DataBaseRouter::CreateRoute(std::string const & from, std::string const & to) {
    proxy_route proxy{router, router->BuildRoute(waiting_stops.at(from).inp, waiting_stops.at(to).inp)};
    if (!proxy.IsValid())
        return nullptr;

    RouteRespType resp = std::make_shared<RouteResponse>();
    resp->items.reserve(proxy.GetInfo()->edge_count);
    for (auto edge_id : proxy.GetRoute()){
        resp->items.push_back(edge_by_bus.at(edge_id));
    }
    resp->total_time = proxy.GetInfo()->weight;
    return resp;
}

std::optional<double> Data_Structure::DataBaseRouter::GetRouteWeight(std::string const & from, std::string const & to) {
    return router->GetRouteWeight(waiting_stops.at(from).inp, waiting_stops.at(to).inp);
}

Data_Structure::DataBaseRouter::proxy_route Data_Structure::DataBaseRouter::GetRoute(std::string const & from, std::string const & to) {
    return proxy_route {router, router->BuildRoute(waiting_stops.at(from).inp, waiting_stops.at(to).inp)};
}

void Data_Structure::DataBaseRouter::Serialize(TCProto::TransportCatalog & tc) const {
    RouterProto::Router router_mes;

    RouterProto::RoutingSettings rs_mes;
    rs_mes.set_bus_wait_time(routing_settings.bus_wait_time);
    rs_mes.set_bus_velocity(routing_settings.bus_velocity);
    *router_mes.mutable_routing_settings() = std::move(rs_mes);

    for (auto & [stop_name, verts] : waiting_stops){
        RouterProto::Vertex vert;
        vert.set_name(stop_name);

        RouterProto::VertInfo vi1;
        vi1.set_vertex_id(verts.inp);
        vi1.set_weight(0);
        *vert.add_route_data_in() = vi1;

        RouterProto::VertInfo vi2;
        vi2.set_vertex_id(verts.out);
        vi2.set_weight(0);
        *vert.add_route_data_out() = vi2;

        *router_mes.add_vertexes() = std::move(vert);
    }

    graph_map.Serialize(router_mes);

    for (auto & edge : *router_mes.mutable_edges()){
        auto & item = edge_by_bus.at(edge.id());

        edge.set_name(item->name);
        if (item->type == RouteResponse::Item::ItemType::BUS){
            RouterProto::SpanCount sc;
            sc.set_count(reinterpret_cast<RouteResponse::Bus const *>(item.get())->span_count);
            *edge.mutable_count() = std::move(sc);
        }
    }

    router->Serialize(router_mes);

    *tc.mutable_router() = std::move(router_mes);
}

Data_Structure::RoutingSettings Data_Structure::DataBaseRouter::GetSettings() const {
    return routing_settings;
}