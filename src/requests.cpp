#include "requests.h"

std::vector<DS::DBItem> ReadBaseRequests(const Json::Node &input) {
    std::vector<DS::DBItem> elements;
    RequestType req_handler;
    for (auto const & el : input.AsArray()) {
        req_handler = CreateRequest(ModifyRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        elements.emplace_back(reinterpret_cast<ModifyRequest *>(req_handler.get())->Process());
    }
    return std::move(elements);
}

DS::RoutingSettings ReadRoutingSettings(const Json::Node &input) {
    return {static_cast<double>(input["bus_wait_time"].AsNumber<int>()), input["bus_velocity"].AsNumber<int>(), input["pedestrian_velocity"].AsNumber<double>()};
}

std::vector<JsonResponse> ReadStatRequests(const DS::DataBase &db, const Json::Node &input) {
    std::vector<JsonResponse> responses;
    RequestType req_handler;
    for (auto const & el : input.AsArray()) {
        req_handler = CreateRequest(ExecuteRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        responses.emplace_back(reinterpret_cast<ExecuteRequest *>(req_handler.get())->Process(db));
    }

    return std::move(responses);
}

// TODO переписать функцию ниже без копипасты и кучи ифов (мб чета с рефлексией)
YellowPages::Database ReadYellowPagesData(const Json::Node &input) {
    YellowPages::Database db;

    for (auto & rubric : input["rubrics"].AsMap()){
        YellowPages::Rubric rubric_mes;

        rubric_mes.set_name(rubric.first);
        for (auto & keys : rubric.second.AsMap()) {
            rubric_mes.add_keywords(keys.second.AsString());
        }
        db.mutable_rubrics()->insert({std::stoi(rubric.first), rubric_mes});
    }

    for (auto & company : input["companies"].AsArray()) {
        YellowPages::Company company_mes;

        if (company.AsMap().count("address")) {
            for (auto &address : company["address"].AsMap()) {
                YellowPages::Address address_mes;
                if (address.first == "formatted") {
                    address_mes.set_formatted(address.second.AsString());
                } else if (address.first == "components") {
                    for (auto &ac : address.second.AsMap()) {
                        YellowPages::AddressComponent ac_mes;
                        ac_mes.set_value(address.second["value"].AsString());
                        // хз че делать с AddressComponent::Type (нет поля для этого enum'a)
                        *address_mes.add_components() = std::move(ac_mes);
                    }
                } else if (address.first == "coords") {
                    SphereProto::Coords coords;
                    coords.set_lat(std::stod(address.second["lat"].AsString()));
                    coords.set_lon(std::stod(address.second["lon"].AsString()));
                    *address_mes.mutable_coords() = std::move(coords);
                } else if (address.first == "comment") {
                    address_mes.set_comment(address.second.AsString());
                }
            }
        }

        if (company.AsMap().count("names")) {
            for (auto &name : company["names"].AsArray()) {
                YellowPages::Name name_mes;
                name_mes.set_value(name["value"].AsString());
                if (name.AsMap().count("type")) {
                    YellowPages::Name_Type type_mes;
                    if (name["type"].AsString() == "MAIN") {
                        type_mes = YellowPages::Name_Type_MAIN;
                    } else if (name["type"].AsString() == "SYNONYM") {
                        type_mes = YellowPages::Name_Type_SYNONYM;
                    } else {
                        type_mes = YellowPages::Name_Type_SHORT;
                    }
                    name_mes.set_type(type_mes);
                }
                *company_mes.add_names() = std::move(name_mes);
            }
        }

        if (company.AsMap().count("phones")) {
            for (auto &phone : company["phones"].AsArray()) {
                YellowPages::Phone phone_mes;
                if (phone.AsMap().count("formatted")) {
                    phone_mes.set_formatted(phone["formatted"].AsString());
                }
                if (phone.AsMap().count("type")) {
                    YellowPages::Phone_Type pt_mes;
                    phone_mes.set_has_type(true);
                    if (phone["type"].AsString() == "FAX") {
                        pt_mes = YellowPages::Phone_Type_FAX;
                    } else if (phone["type"].AsString() == "PHONE") {
                        pt_mes = YellowPages::Phone_Type_PHONE;
                    } else {
                        phone_mes.set_has_type(false);
                    }
                    phone_mes.set_type(pt_mes);
                }
                if (phone.AsMap().count("country_code")) {
                    phone_mes.set_country_code(phone["country_code"].AsString());
                }
                if (phone.AsMap().count("local_code")) {
                    phone_mes.set_local_code(phone["local_code"].AsString());
                }
                if (phone.AsMap().count("number")) {
                    phone_mes.set_number(phone["number"].AsString());
                }
                if (phone.AsMap().count("extension")) {
                    phone_mes.set_extension(phone["extension"].AsString());
                }
                if (phone.AsMap().count("description")) {
                    phone_mes.set_description(phone["description"].AsString());
                }

                *company_mes.add_phones() = std::move(phone_mes);
            }
        }

        if (company.AsMap().count("urls")) {
            for (auto &url : company["urls"].AsArray()) {
                YellowPages::Url url_mes;
                url_mes.set_value(url["value"].AsString());
                *company_mes.add_urls() = std::move(url_mes);
            }
        }

        if (company.AsMap().count("rubrics")) {
            for (auto &rubric : company["rubrics"].AsArray()) {
                company_mes.add_rubrics(rubric.AsNumber<int>());
            }
        }

        if (company.AsMap().count("working_time")) {
            auto &wt = company["working_time"];
            YellowPages::WorkingTime wt_mes;
            if (wt.AsMap().count("formatted")) {
                wt_mes.set_formatted(wt["formatted"].AsString());
            }
            for (auto &wti : wt["intervals"].AsArray()) {
                YellowPages::WorkingTimeInterval wti_mes;
                if (wti.AsMap().count("day")) {
                    YellowPages::WorkingTimeInterval::Day d;
                    if (wti["day"].AsString() == "EVERYDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_EVERYDAY;
                    } else if (wti["day"].AsString() == "MONDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_MONDAY;
                    } else if (wti["day"].AsString() == "TUESDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_TUESDAY;
                    } else if (wti["day"].AsString() == "WEDNESDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_WEDNESDAY;
                    } else if (wti["day"].AsString() == "THURSDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_THURSDAY;
                    } else if (wti["day"].AsString() == "FRIDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_FRIDAY;
                    } else if (wti["day"].AsString() == "SATURDAY") {
                        d = YellowPages::WorkingTimeInterval_Day_SATURDAY;
                    } else {
                        d = YellowPages::WorkingTimeInterval_Day_SUNDAY;
                    }
                    wti_mes.set_day(d);
                }
                if (wti.AsMap().count("minutes_from")) {
                    wti_mes.set_minutes_from(wti["minutes_from"].AsNumber<int>());
                }
                if (wti.AsMap().count("minutes_to")) {
                    wti_mes.set_minutes_to(wti["minutes_to"].AsNumber<int>());
                }
                *wt_mes.add_intervals() = std::move(std::move(wti_mes));
            }
        }

        if (company.AsMap().count("nearby_stops")) {
            for (auto &nearby_stop : company["nearby_stops"].AsArray()) {
                YellowPages::NearbyStop nearby_stop_mes;
                nearby_stop_mes.set_name(nearby_stop["name"].AsString());
                nearby_stop_mes.set_meters(nearby_stop["meters"].AsNumber<int>());
                *company_mes.add_nearby_stops() = std::move(nearby_stop_mes);
            }
        }

        *db.add_companies() = std::move(company_mes);
    }

    return std::move(db);
}

YellowPages::Rubric ParseRubric(const Json::Node &json_node) {
    YellowPages::Rubric rubric_mes;

    rubric_mes.add_keywords(json_node.AsString());
    return rubric_mes;
}

YellowPages::Name ParseName(const Json::Node &json_node) {
    YellowPages::Name name_mes;

    name_mes.set_value(json_node.AsString());
    return name_mes;
}

YellowPages::Phone ParsePhone(const Json::Node &json_node) {
    YellowPages::Phone phone_mes;

    if (json_node.AsMap().count("type")){
        YellowPages::Phone_Type pt_mes;
        phone_mes.set_has_type(true);
        if (json_node["type"].AsString() == "PHONE") {
            pt_mes = YellowPages::Phone_Type_PHONE;
        } else if (json_node["type"].AsString() == "FAX") {
            pt_mes = YellowPages::Phone_Type_FAX;
        } else {
            phone_mes.set_has_type(false);
        }
        phone_mes.set_type(pt_mes);
    }
    if (json_node.AsMap().count("country_code")) {
        phone_mes.set_country_code(json_node["country_code"].AsString());
    }
    if (json_node.AsMap().count("local_code")){
        phone_mes.set_local_code(json_node["local_code"].AsString());
    }
    if (json_node.AsMap().count("number")){
        phone_mes.set_number(json_node["number"].AsString());
    }
    if (json_node.AsMap().count("extension")) {
        phone_mes.set_extension(json_node["extension"].AsString());
    }
    if (json_node.AsMap().count("description")){
        phone_mes.set_description(json_node["description"].AsString());
    }
    return phone_mes;
}

YellowPages::Url ParseUrl(const Json::Node &json_node) {
    YellowPages::Url url;

    url.set_value(json_node.AsString());
    return url;
}

DS::DBItem AddStopRequest::Process() {
   return DS::Stop{std::move(title), dist, std::move(adjacent_stops)};
}

DS::DBItem AddBusRequest::Process() {
    return DS::Bus{std::move(title), std::move(stops), is_roundtrip};
}

JsonResponse FindBusRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindBus, std::ref(db), std::ref(title));
}

JsonResponse FindStopRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindStop, std::ref(db), std::ref(title));
}

JsonResponse FindRouteRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindRoute, std::ref(db), std::ref(from), std::ref(to));
}

JsonResponse MapRouteRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::BuildMap, std::ref(db));
}

JsonResponse FindCompaniesRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindCompanies, std::ref(db), std::ref(querys));
}

JsonResponse FindRouteToCompaniesRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindRouteToCompanies, std::ref(db), std::ref(from), std::ref(querys));
}

RequestType CreateRequest(IRequest::Type type) {
    switch(type){
        case IRequest::Type::READ_BUS:
            return std::make_unique<AddBusRequest>();
        case IRequest::Type::READ_STOP:
            return std::make_unique<AddStopRequest>();
        case IRequest::Type::FIND_BUS:
            return std::make_unique<FindBusRequest>();
        case IRequest::Type::FIND_STOP:
            return std::make_unique<FindStopRequest>();
        case IRequest::Type::FIND_ROUTE:
            return std::make_unique<FindRouteRequest>();
        case IRequest::Type::BUILD_MAP:
            return std::make_unique<MapRouteRequest>();
        case IRequest::Type::FIND_COMPANIES:
            return std::make_unique<FindCompaniesRequest>();
        case IRequest::Type::FIND_ROUTE_COMPANY:
            return std::make_unique<FindRouteToCompaniesRequest>();
    }
    throw std::logic_error("Bad type");
}

#define METHOD(property, m) ren_set.property = render_settings[#property].m
DS::RenderSettings ReadRenderSettings(const Json::Node &input) {
        auto render_settings = input.AsMap();
        DS::RenderSettings ren_set;

        METHOD(width, AsNumber<double>());
        METHOD(height, AsNumber<double>());
        METHOD(padding, AsNumber<double>());
        METHOD(outer_margin, AsNumber<double>());
        METHOD(stop_radius, AsNumber<double>());
        METHOD(line_width, AsNumber<double>());
        METHOD(company_radius, AsNumber<double>());
        METHOD(company_line_width, AsNumber<double>());

        METHOD(stop_label_font_size, AsNumber<int>());
        METHOD(underlayer_width, AsNumber<double>());

        METHOD(bus_label_font_size, AsNumber<int>());

        size_t i = 0;
        for (auto const & el : render_settings["stop_label_offset"].AsArray()){
            ren_set.stop_label_offset[i++] = el.AsNumber<double>();
        }

        i = 0;
        for (auto const & el : render_settings["bus_label_offset"].AsArray()){
            ren_set.bus_label_offset[i++] = el.AsNumber<double>();
        }

        auto get_color = [](const Json::Node & r_s) {
            Svg::Color color;
            if (std::holds_alternative<std::vector<Json::Node>>(r_s)) {
                auto rgba = r_s.AsArray();
                Svg::Rgba rgb{static_cast<uint8_t>(rgba[0].AsNumber<int>()),
                              static_cast<uint8_t>(rgba[1].AsNumber<int>()),
                              static_cast<uint8_t>(rgba[2].AsNumber<int>())};
                if (rgba.size() == 4) {
                    rgb.alpha.emplace(rgba[3].AsNumber<double>());
                }
                color = Svg::Color(rgb);
            } else {
                color = Svg::Color(r_s.AsString());
            }
            return color;
        };

        ren_set.underlayer_color = get_color(render_settings["underlayer_color"]);
        for (auto const & el : render_settings["color_palette"].AsArray()) {
            ren_set.color_palette.emplace_back(get_color(el));
        }

        for (auto const & el : render_settings["layers"].AsArray()){
            ren_set.layers.emplace_back(el.AsString());
        }

        return std::move(ren_set);
}