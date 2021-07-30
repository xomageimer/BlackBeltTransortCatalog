#include "requests.h"

std::vector<DS::DBItem> ReadBaseRequests(const Json::Node &input) {
    std::vector<DS::DBItem> elements;
    RequestType req_handler;
    for (auto const & el : input.AsArray()){
        req_handler = CreateRequest(ModifyRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        elements.emplace_back(reinterpret_cast<ModifyRequest *>(req_handler.get())->Process());
    }
    return std::move(elements);
}

std::pair<double, int> ReadRoutingSettings(const Json::Node &input) {
    return {static_cast<double>(input["bus_wait_time"].AsNumber<int>()), input["bus_velocity"].AsNumber<int>()};
}

std::vector<JsonResponse> ReadStatRequests(const DS::DataBase &db, const Json::Node &input) {
    std::vector<JsonResponse> responses;
    RequestType req_handler;
    for (auto const & el : input.AsArray()){
        req_handler = CreateRequest(ExecuteRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        responses.emplace_back(reinterpret_cast<ExecuteRequest *>(req_handler.get())->Process(db));
    }

    return std::move(responses);
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
