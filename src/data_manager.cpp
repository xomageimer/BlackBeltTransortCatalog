#include "data_manager.h"

#include "transport_catalog.pb.h"

#include <algorithm>

Data_Structure::DataBase::DataBase(std::istream & is) {
    Deserialize(is);
}

Data_Structure::DataBase::DataBase(const std::vector<DBItem>& elems, const std::pair<double, int> routing_settings_) {
    const auto [stops_ptrs, buses_ptrs] = Init(elems);

    router = std::make_unique<DataBaseRouter>(
            stops_ptrs,
            buses_ptrs,
            routing_settings_
    );
}

Data_Structure::DataBase::DataBase(std::vector<DBItem> items, std::pair<double, int> routing_settings_,
                                   RenderSettings render_settings) {
    const auto [stops_ptrs, buses_ptrs] = Init(items);

    router = std::make_unique<DataBaseRouter>(
            stops_ptrs,
            buses_ptrs,
            routing_settings_
    );

    svg_builder = std::make_unique<DataBaseSvgBuilder>(
            std::move(render_settings)
    );
}

std::pair<Dict<Data_Structure::Stop>, Dict<Data_Structure::Bus>> Data_Structure::DataBase::Init(std::vector<DBItem> const & elems) {
    Dict<Stop> stops_ptrs;
    Dict<Bus> buses_ptrs;
    for (auto & el : elems){
        if (std::holds_alternative<Bus>(el)){
            const auto & bus = std::get<Bus>(el);
            buses_ptrs[bus.name] = &bus;
        } else {
            const auto & stop = std::get<Stop>(el);
            stops_ptrs.emplace(stop.name, &stop);
            stops.emplace(stop.name, std::make_shared<StopResponse>());
        }
    }

    try {
        for (auto[_, bus] : buses_ptrs) {
            BusRespType bus_resp = std::make_shared<BusResponse>();
            bus_resp->stop_count = bus->stops.size();
            bus_resp->unique_stop_count = Ranges::GetUniqueItems(Ranges::AsRange(bus->stops));
            bus_resp->length = ComputeRouteDistance(bus->stops, stops_ptrs);
            bus_resp->curvature = bus_resp->length / ComputeGeoDistance(bus->stops, stops_ptrs);
            buses.emplace(bus->name, std::move(bus_resp));

            for (auto &el : !bus->is_roundtrip
                            ? Ranges::ToMiddle(Ranges::AsRange(bus->stops))
                            : Ranges::AsRange(bus->stops)) {
                stops[el]->buses.emplace(bus->name);
            }
        }
    } catch (...) {
        std::cout << "The DB condition is violated\n";
    }

    return {stops_ptrs, buses_ptrs};
}

ResponseType Data_Structure::DataBase::FindBus(const std::string &title) const {
    auto ret = buses.find(title);
    if (ret != buses.end()) {
        return ret->second;
    }
    else return GenerateBad();
}

ResponseType Data_Structure::DataBase::FindStop(const std::string &title) const {
    auto ret = stops.find(title);
    if (ret != stops.end()) {
        return ret->second;
    }
    else return GenerateBad();
}

ResponseType Data_Structure::DataBase::FindRoute(const std::string &from, const std::string &to) const {
    auto ret = router->CreateRoute(from, to);
    if (ret) {
        auto render_items = ret->items;
        if (!render_items.empty()) {
            auto finish = render_items.insert(render_items.end(), std::make_shared<RouteResponse::Wait>());
            (*finish)->name = to;
        }
        ret->route_render = std::move(svg_builder->RenderRoute(render_items)->svg_xml_answer);

        return ret;
    }
    else return GenerateBad();
}

ResponseType Data_Structure::DataBase::BuildMap() const {
    if (!svg_builder)
        return GenerateBad();
    return svg_builder->RenderMap();
}

ResponseType Data_Structure::DataBase::GenerateBad() {
    auto ret = std::make_shared<BadResponse>();
    ret->error_message = "not found";
    return ret;
}

int Data_Structure::ComputeStopsDistance(const Data_Structure::Stop &lhs, const Data_Structure::Stop &rhs) {
    if (auto it = lhs.adjacent_stops.find(rhs.name); it != lhs.adjacent_stops.end()) {
        return it->second;
    } else {
        return rhs.adjacent_stops.at(lhs.name);
    }
}

int Data_Structure::ComputeRouteDistance(const std::vector<std::string> &stops, const Dict<Data_Structure::Stop> & db_stops) {
    int result = 0;
    for (size_t i = 1; i < stops.size(); i++) {
        result += ComputeStopsDistance(*db_stops.at(stops[i - 1]), *db_stops.at(stops[i]));
    }
    return result;
}

double Data_Structure::ComputeGeoDistance(const std::vector<std::string> &stops, const Dict<Data_Structure::Stop> & db_stops) {
    double result = 0.;
    for (size_t i = 1; i < stops.size(); i++){
        result += ComputeDistance(db_stops.at(stops.at(i - 1))->dist,
                                  db_stops.at(stops.at(i))->dist);
    }
    return result;
}

std::set<std::string>
Data_Structure::GetBearingPoints(const Dict<Data_Structure::Stop> &stops, const Dict<Data_Structure::Bus> &buses) {
    std::set<std::string> res;
    std::map<std::string, std::string> stop_and_buses;
    std::map<std::string, int> stops_count;

    for (auto & [stop_name, _] : stops){
        stops_count[stop_name] = 0;
    }

    for (auto const & [_, bus] : buses){
        if (!bus->stops.empty()) {
            res.insert(bus->stops.front());
            if (!bus->is_roundtrip) {
                res.insert(*(std::prev(Ranges::ToMiddle(Ranges::AsRange(bus->stops)).end())));
            }
        }
        for (auto const & stop_name : bus->stops){
            ++stops_count[stop_name];
            const auto [it, inserted] = stop_and_buses.emplace(stop_name, bus->name);
            if (!inserted && it->second != bus->name)
                res.insert(stop_name);
        }
    }

    for (const auto& [stop, count] : stops_count) {
        if (count > 2 || count == 0) {
            res.insert(stop);
        }
    }

    return res;
}

void Data_Structure::DataBase::Serialize(std::ostream &os) const {
    Serialize::TransportCatalog tc;

    for (auto & stop : stops){
        Serialize::Stop dummy_stop;
        dummy_stop.set_name(stop.first);
        for (auto & bus_name : stop.second->buses){
            dummy_stop.add_buses(bus_name);
        }
        *tc.add_stops() = std::move(dummy_stop);
    }

    for (auto & [bus_name, bus] : buses){
        Serialize::Bus dummy_bus;
        dummy_bus.set_name(bus_name);
        dummy_bus.set_length(bus->length);
        dummy_bus.set_curvature(bus->curvature);
        dummy_bus.set_stop_count(bus->stop_count);
        dummy_bus.set_unique_stop_count(bus->unique_stop_count);

        *tc.add_buses() = std::move(dummy_bus);
    }

    router->Serialize(tc);
    svg_builder->Serialize(tc);

    tc.SerializePartialToOstream(&os);
}

void Data_Structure::DataBase::Deserialize(std::istream &is) {
    Serialize::TransportCatalog tc;
    tc.ParseFromIstream(&is);

    for (auto & stop : tc.stops()){
        std::set<std::string> s_buses;
        for (auto & bus_name : stop.buses()){
            s_buses.insert(bus_name);
        }
        StopResponse sr;
        sr.buses = std::move(s_buses);
        this->stops.emplace(stop.name(), std::make_shared<StopResponse>(std::move(sr)));
    }

    for (auto & bus : tc.buses()){
        BusResponse br;
        br.length = bus.length(); br.curvature = bus.curvature(); br.stop_count = bus.stop_count(); br.unique_stop_count = bus.unique_stop_count();
        this->buses.emplace(bus.name(), std::make_shared<BusResponse>(std::move(br)));
    }

    router = std::make_unique<DataBaseRouter>(tc.router());
    svg_builder = std::make_unique<DataBaseSvgBuilder>(tc.render());
}