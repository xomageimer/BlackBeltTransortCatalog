#include "data_manager.h"

#include "transport_catalog.pb.h"

#include <algorithm>

Data_Structure::DataBase::DataBase(std::istream & is) {
    Deserialize(is);
}

Data_Structure::DataBase::DataBase(const std::vector<DBItem>& elems, const RoutingSettings routing_settings_) {
    Init(elems);

    router = std::make_unique<DataBaseRouter>(
            pure_stops,
            pure_buses,
            routing_settings_
    );
}

Data_Structure::DataBase::DataBase(std::vector<DBItem> items, RoutingSettings routing_settings_,
                                   RenderSettings render_settings){
    Init(items);

    router = std::make_unique<DataBaseRouter>(
            pure_stops,
            pure_buses,
            routing_settings_
    );

    svg_builder = std::make_unique<DataBaseSvgBuilder>(
            std::move(render_settings)
    );
}

Data_Structure::DataBase::DataBase(std::vector<DBItem> items, YellowPages::Database yellow_pages, RoutingSettings routing_settings_,
                                   Data_Structure::RenderSettings render_settings) {
    Init(items);

    yellow_pages_db = std::make_unique<DataBaseYellowPages>(std::move(yellow_pages));

    router = std::make_unique<DataBaseRouter>(
            pure_stops,
            pure_buses,
            routing_settings_
    );

    svg_builder = std::make_unique<DataBaseSvgBuilder>(
            std::move(render_settings)
    );
}

void Data_Structure::DataBase::Init(std::vector<DBItem> const & elems) {
    for (auto & el : elems){
        if (std::holds_alternative<Bus>(el)){
            const auto & bus = std::get<Bus>(el);
            pure_buses.emplace(bus.name, bus);
        } else {
            const auto & stop = std::get<Stop>(el);
            pure_stops.emplace(stop.name, stop);
            stops.emplace(stop.name, std::make_shared<StopResponse>());
        }
    }
    try {
        for (auto[_, bus] : pure_buses) {
            BusRespType bus_resp = std::make_shared<BusResponse>();
            bus_resp->stop_count = bus.stops.size();
            bus_resp->unique_stop_count = Ranges::GetUniqueItems(Ranges::AsRange(bus.stops));
            bus_resp->length = ComputeRouteDistance(bus.stops, pure_stops);
            bus_resp->curvature = bus_resp->length / ComputeGeoDistance(bus.stops, pure_stops);
            buses.emplace(bus.name, std::move(bus_resp));

            for (auto &el : !bus.is_roundtrip
                            ? Ranges::ToMiddle(Ranges::AsRange(bus.stops))
                            : Ranges::AsRange(bus.stops)) {
                stops[el]->buses.emplace(bus.name);
            }
        }
    } catch (...) {
        std::cout << "The DB condition is violated\n";
    }
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

ResponseType Data_Structure::DataBase::FindCompanies(const std::vector<std::shared_ptr<Query>> & querys) const {
    if (!yellow_pages_db)
        return GenerateBad();
    auto ret = yellow_pages_db->FindCompanies(querys);
    if (!ret)
        return GenerateBad();

    return ret;
}

ResponseType Data_Structure::DataBase::FindRouteToCompanies(const std::string &from,
                                                            const std::vector<std::shared_ptr<Query>> &querys) const {
    auto resp = FindCompanies(querys);
    if (!resp)
        return GenerateBad();

    double route_time = 0;
    double time_from_stop = 0;
    YellowPages::Company const * company = nullptr;
    std::string nearby_stop;
    for (auto company_ptr : reinterpret_cast<CompaniesResponse const *>(resp.get())->companies){
        for (auto & stop : company_ptr->nearby_stops()){
            auto cur_route_time = router->GetRouteWeight(from, stop.name());
            double time_to_cur = ComputeTimeToWalking(stop.meters(), router->GetSettings().pedestrian_velocity);
            if (!company || route_time + time_from_stop > *cur_route_time + time_to_cur) {
                company = company_ptr;
                route_time = *cur_route_time;
                time_from_stop = time_to_cur;
                nearby_stop = stop.name();
            }
        }
    }

    auto result_resp = std::make_shared<RouteToCompaniesResponse>();
    std::shared_ptr<RouteResponse> route_resp = result_resp;
    route_resp = router->CreateRoute(from, nearby_stop);

    result_resp->total_time = route_resp->total_time + time_from_stop;
    result_resp->time_to_walk = time_from_stop;
    result_resp->nearby_stop_name = nearby_stop;
    for (auto & name : company->names()){
        if (name.type() == YellowPages::Name_Type_MAIN){
            result_resp->company_full_name = name.value();
            if (!company->rubrics().empty()) {
                std::string new_name = yellow_pages_db->GetRubric(company->rubrics(0)).keywords(0) + " " + result_resp->company_full_name;
                result_resp->company_full_name = new_name;
            }
            break;
        }
    }

    auto render_items = result_resp->items;
    if (!render_items.empty()) {
        auto finish = render_items.insert(render_items.end(), std::make_shared<RouteResponse::Wait>());
        (*finish)->name = nearby_stop;
    }
    result_resp->route_render = std::move(svg_builder->RenderPathToCompany(render_items, result_resp->company_full_name)->svg_xml_answer);

    return std::move(result_resp);
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

int Data_Structure::ComputeRouteDistance(const std::vector<std::string> &stops, const std::unordered_map<std::string, Stop> & db_stops) {
    int result = 0;
    for (size_t i = 1; i < stops.size(); i++) {
        result += ComputeStopsDistance(db_stops.at(stops[i - 1]), db_stops.at(stops[i]));
    }
    return result;
}

double Data_Structure::ComputeGeoDistance(const std::vector<std::string> &stops, const std::unordered_map<std::string, Stop> & db_stops) {
    double result = 0.;
    for (size_t i = 1; i < stops.size(); i++){
        result += ComputeDistance(db_stops.at(stops.at(i - 1)).dist,
                                  db_stops.at(stops.at(i)).dist);
    }
    return result;
}

std::set<std::string>
Data_Structure::GetBearingPoints(const std::unordered_map<std::string, stop_n_companies> &stops, const std::unordered_map<std::string, Bus> &buses) {
    std::set<std::string> res;
    std::map<std::string, std::string> stop_and_buses;
    std::map<std::string, int> stops_count;

    for (auto & [stop_name, _] : stops){
        stops_count[stop_name] = 0;
    }

    for (auto const & [_, bus] : buses){
        if (!bus.stops.empty()) {
            res.insert(bus.stops.front());
            if (!bus.is_roundtrip) {
                res.insert(*(std::prev(Ranges::ToMiddle(Ranges::AsRange(bus.stops)).end())));
            }
        }
        for (auto const & stop_name : bus.stops){
            ++stops_count[stop_name];
            const auto [it, inserted] = stop_and_buses.emplace(stop_name, bus.name);
            if (!inserted && it->second != bus.name)
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

double Data_Structure::ComputeTimeToWalking(double meters, double walk_speed) {
    return meters / walk_speed;
}

void Data_Structure::DataBase::Serialize(std::ostream &os) const {
    TCProto::TransportCatalog tc;

    for (auto & stop : stops){
        TCProto::Stop dummy_stop;
        dummy_stop.set_name(stop.first);
        for (auto & bus_name : stop.second->buses){
            dummy_stop.add_buses(bus_name);
        }

        auto & pure_stop = pure_stops.at(stop.first);
        dummy_stop.set_latitude(pure_stop.dist.GetLatitude());
        dummy_stop.set_longitude(pure_stop.dist.GetLongitude());
        for (auto & el : pure_stop.adjacent_stops){
            TCProto::AdjacentStops as;
            as.set_name(el.first);
            as.set_dist(el.second);
            *dummy_stop.add_adjacent_stops() = std::move(as);
        }

        *tc.add_stops() = std::move(dummy_stop);
    }

    for (auto & [bus_name, bus] : buses){
        TCProto::Bus dummy_bus;
        dummy_bus.set_name(bus_name);
        dummy_bus.set_length(bus->length);
        dummy_bus.set_curvature(bus->curvature);
        dummy_bus.set_stop_count(bus->stop_count);
        dummy_bus.set_unique_stop_count(bus->unique_stop_count);

        auto & pure_bus = pure_buses.at(bus_name);
        dummy_bus.set_is_roundtrip(pure_bus.is_roundtrip);
        for (auto & el : pure_bus.stops){
            dummy_bus.add_stops(el);
        }

        *tc.add_buses() = std::move(dummy_bus);
    }

    *tc.mutable_yellow_pages() = yellow_pages_db->Serialize();
    router->Serialize(tc);
    svg_builder->Serialize(tc);

    tc.SerializePartialToOstream(&os);
}

void Data_Structure::DataBase::Deserialize(std::istream &is) {
    TCProto::TransportCatalog tc;
    tc.ParseFromIstream(&is);

    for (auto & stop : tc.stops()){
        std::set<std::string> s_buses;
        for (auto & bus_name : stop.buses()){
            s_buses.insert(bus_name);
        }
        StopResponse sr;
        sr.buses = std::move(s_buses);
        this->stops.emplace(stop.name(), std::make_shared<StopResponse>(std::move(sr)));

        Stop pure_stop;
        pure_stop.name = stop.name();
        pure_stop.dist = Distance{stop.longitude(), stop.latitude()};
        for (auto & as : stop.adjacent_stops()){
            pure_stop.adjacent_stops.emplace(as.name(), as.dist());
        }
        pure_stops.emplace(pure_stop.name, pure_stop);
    }

    for (auto & bus : tc.buses()){
        BusResponse br;
        br.length = bus.length(); br.curvature = bus.curvature(); br.stop_count = bus.stop_count(); br.unique_stop_count = bus.unique_stop_count();
        this->buses.emplace(bus.name(), std::make_shared<BusResponse>(std::move(br)));

        Bus pure_bus;
        pure_bus.name = bus.name();
        pure_bus.is_roundtrip = bus.is_roundtrip();
        for (auto & stop : bus.stops()){
            pure_bus.stops.emplace_back(stop);
        }
        pure_buses.emplace(pure_bus.name, pure_bus);
    }

    yellow_pages_db = std::make_unique<DataBaseYellowPages>(tc.yellow_pages());

    router = std::make_unique<DataBaseRouter>(tc.router());

    std::unordered_map<std::string, const YellowPages::Company *> companies_map;
    for (auto & company : yellow_pages_db->GetOrigin().companies()){
        std::string company_name;
        for (auto & name : company.names()){
            if (name.type() == YellowPages::Name_Type_MAIN) {
                company_name += name.value();
                break;
            }
        }
        if (!company.rubrics().empty()) {
            std::string new_name = yellow_pages_db->GetRubric(company.rubrics(0)).keywords(0) + " " + company_name;
            company_name = new_name;
        }
        companies_map.emplace(company_name, &company);
    }
    svg_builder = std::make_unique<DataBaseSvgBuilder>(tc.render(), pure_stops, pure_buses, companies_map);
}