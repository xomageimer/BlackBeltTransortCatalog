#include "render_manager.h"
#include "data_manager.h"

#include <cmath>

Data_Structure::MapRespType Data_Structure::DataBaseSvgBuilder::RenderMap() const {
    MapRespType MapResp = std::make_shared<MapResponse>();
    MapResp->svg_xml_answer = doc.Get();
    return MapResp;
}

Data_Structure::MapRespType Data_Structure::DataBaseSvgBuilder::RenderRoute(std::vector<RouteResponse::ItemPtr> const & items) {
    auto route_doc = prerender_doc;

    Svg::Rect rect;
    auto left_corner = Svg::Point{-renderSettings.outer_margin, -renderSettings.outer_margin};
    rect.SetFillColor(renderSettings.underlayer_color)
            .SetPoint(left_corner)
            .SetWidth(renderSettings.width + renderSettings.outer_margin - left_corner.x)
            .SetHeight(renderSettings.height + renderSettings.outer_margin - left_corner.y);
    route_doc.Add(rect);

    if (!items.empty()) {
        std::vector<std::string> route_coords;
        std::vector<std::pair<std::string, size_t>> used_bus;
        for (auto &item : items) {
            if (item->type == RouteResponse::Item::ItemType::WAIT)
                route_coords.push_back(item->name);
            else
                used_bus.emplace_back(
                        std::pair{item->name, reinterpret_cast<RouteResponse::Bus *>(item.get())->span_count});
        }
        for (const auto &layer : renderSettings.layers) {
            (layersStrategy[layer])->DrawPartial(this, route_coords, used_bus, route_doc);
        }
    }

    route_doc.SimpleRender();
    MapRespType MapResp = std::make_shared<MapResponse>();
    MapResp->svg_xml_answer = route_doc.Get();
    return MapResp;
}

Data_Structure::DataBaseSvgBuilder::DataBaseSvgBuilder(const Dict<Data_Structure::Stop> &stops,
                                       const Dict<Data_Structure::Bus> &buses,
                                       RenderSettings render_set) : renderSettings(std::move(render_set)){
    CalculateCoordinates(stops, buses);
    Init(buses);
    for (const auto & layer : renderSettings.layers) {
        (layersStrategy[layer])->Draw(this);
    }

    prerender_doc = doc;
    doc.SimpleRender();
}

std::map<std::string, Svg::Point> Data_Structure::DataBaseSvgBuilder::CoordinateUniformDistribution(const Dict<Data_Structure::Stop> &stops,
                                                                       const Dict<Data_Structure::Bus> &buses) {
    auto bearing_points = GetBearingPoints(stops, buses);

    auto step = [](double left, double right, size_t count) {
        return (right - left) / count;
    };

    std::map<std::string, Svg::Point> uniform;
    for (auto & [_, bus] : buses) {
        if (bus->stops.empty()) continue;
        auto left_bearing_point = bus->stops.begin();
        size_t l = 0;
        size_t k = l;
        auto right_bearing_point = left_bearing_point;
        size_t count;
        auto bus_range = bus->is_roundtrip ? Ranges::AsRange(bus->stops) : Ranges::ToMiddle(Ranges::AsRange(bus->stops));
        for (auto stop_iter = bus_range.begin(); stop_iter != bus_range.end(); stop_iter++) {
            count = std::distance(left_bearing_point, right_bearing_point);
            if (stop_iter == right_bearing_point) {
                left_bearing_point = right_bearing_point;
                l = k;
                right_bearing_point = std::find_if(std::next(left_bearing_point), bus_range.end(), [&bearing_points](auto const & cur) {
                    return bearing_points.find(cur) != bearing_points.end();
                });
                auto left_bearing_distance = stops.at(*left_bearing_point)->dist;
                uniform.insert_or_assign(*left_bearing_point, Svg::Point{left_bearing_distance.GetLongitude(), left_bearing_distance.GetLatitude()});
            } else {
                if (right_bearing_point == bus_range.end())
                    throw std::logic_error("bad right point!");

                auto left_bearing_distance = stops.at(*left_bearing_point)->dist;
                auto right_bearing_distance = stops.at(*right_bearing_point)->dist;

                double x = left_bearing_distance.GetLongitude()
                           + step(left_bearing_distance.GetLongitude(),
                                  right_bearing_distance.GetLongitude(), count)
                             * static_cast<double>(k - l);
                double y = left_bearing_distance.GetLatitude()
                           + step(left_bearing_distance.GetLatitude(),
                                  right_bearing_distance.GetLatitude(), count)
                             * static_cast<double>(k - l);
                uniform.insert_or_assign(*stop_iter, Svg::Point{x, y});
            }
            ++k;
        }
    }

    BuildNeighborhoodConnections(uniform, buses);
    return uniform;
}

auto Data_Structure::DataBaseSvgBuilder::SortingByCoordinates(const std::map<std::string, Svg::Point> &uniform,
                                                              const Dict<Data_Structure::Stop> &stops) {
    std::pair<std::vector<std::pair<double, std::string>>, std::vector<std::pair<double, std::string>>> sorted_xy;
    auto & sorted_x = sorted_xy.first;
    auto & sorted_y = sorted_xy.second;
    for (auto & [stop_name, stop] : stops)
    {
        auto it = uniform.find(stop_name);
        if (it != uniform.end()) {
            auto &[name, new_coord] = *it;
            sorted_x.emplace_back(new_coord.x, stop_name);
            sorted_y.emplace_back(new_coord.y, stop_name);
        } else {
            sorted_x.emplace_back(stop->dist.GetLongitude(), stop_name);
            sorted_y.emplace_back(stop->dist.GetLatitude(), stop_name);
        }
    }
    std::sort(sorted_x.begin(), sorted_x.end());
    std::sort(sorted_y.begin(), sorted_y.end());
    return std::move(sorted_xy);
}

std::pair<std::map<std::string, int>, int> Data_Structure::DataBaseSvgBuilder::GluingCoordinates(
        const std::vector<std::pair<double, std::string>> &sorted_by_coord) {

    std::map<std::string, int> gluing;
    int idx_max = 0;

    int idx = 0;

    auto refresh = [&idx, this, &gluing](std::string const & stop){
        int potential_id = 0;
        bool was_checked = false;
        auto it = db_connected.find(stop);
        if (it != db_connected.end()) {
            for (auto &cur_stop : it->second) {
                auto cur_it = gluing.find(cur_stop);
                if (cur_it != gluing.end()) {
                    was_checked = true;
                    potential_id = std::max(potential_id, cur_it->second);
                }
            }
        }
        idx = was_checked ? potential_id + 1 : 0;
    };

    auto beg_of_cur_idx = sorted_by_coord.begin();
    for (const auto & it_x : sorted_by_coord) {
        refresh(it_x.second);
        gluing.emplace(it_x.second, idx);
        if (idx > idx_max)
            idx_max = idx;
    }

    return {std::move(gluing), idx_max};

}

std::map<std::string, Svg::Point> Data_Structure::DataBaseSvgBuilder::CoordinateCompression(const Dict<Data_Structure::Stop> &stops, const Dict<Data_Structure::Bus> & buses) {
    auto [sorted_x, sorted_y] = SortingByCoordinates(CoordinateUniformDistribution(stops, buses), stops);

    auto [gluing_y, yid] = GluingCoordinates(sorted_y);
    auto [gluing_x, xid] = GluingCoordinates(sorted_x);
    double x_step = xid ? (renderSettings.width - 2.f * renderSettings.padding) / xid : 0;
    double y_step = yid ? (renderSettings.height - 2.f * renderSettings.padding) / yid : 0;

    std::map<std::string, double> new_x;
    std::map<std::string, double> new_y;
    for (auto [coord, stop_name] : sorted_y) {
        new_y.emplace(stop_name, renderSettings.height - renderSettings.padding - (gluing_y.at(stop_name)) * y_step);
    }
    for (auto [coord, stop_name]: sorted_x) {
        new_x.emplace(stop_name, (gluing_x.at(stop_name)) * x_step + renderSettings.padding);
    }

    std::map<std::string, Svg::Point> CompressCoord;
    for (auto & [stop_name, _] : stops){
        CompressCoord.emplace(stop_name, Svg::Point{new_x.at(stop_name), new_y.at(stop_name)});
    }
    return CompressCoord;
}

void Data_Structure::DataBaseSvgBuilder::CalculateCoordinates(const Dict<Data_Structure::Stop> &stops,
                                                              const Dict<Data_Structure::Bus> & buses) {
    stops_coordinates = CoordinateCompression(stops, buses);
}

void Data_Structure::DataBaseSvgBuilder::Init(const Dict<Data_Structure::Bus> &buses) {
    size_t size = renderSettings.color_palette.size();
    size_t i = 0;
    for (auto & [bus_name, bus] : buses) {
        bus_dict.emplace(bus_name, std::pair{*bus, renderSettings.color_palette[i++ % size]});
    }

    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("bus_lines"),
                           std::forward_as_tuple(std::make_shared<BusPolylinesDrawer>(bus_dict)));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("bus_labels"),
                           std::forward_as_tuple(std::make_shared<BusTextDrawer>(bus_dict)));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("stop_points"),
                           std::forward_as_tuple(std::make_shared<StopsRoundDrawer>()));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("stop_labels"),
                           std::forward_as_tuple(std::make_shared<StopsTextDrawer>()));

}

void Data_Structure::DataBaseSvgBuilder::BuildNeighborhoodConnections( std::map<std::string, Svg::Point> const & new_coords, const Dict<struct Bus> &buses) {
    for (auto & [_, bus] : buses) {
        auto & stops_ = bus->stops;
        if (stops_.empty()) continue;
        for (auto stops_it = stops_.begin(); stops_it != std::prev(stops_.end()); stops_it++) {
            db_connected[*stops_it].insert(*std::next(stops_it));
            db_connected[*std::next(stops_it)].insert(*stops_it);
        }
    }
}

bool Data_Structure::DataBaseSvgBuilder::IsConnected(std::string const & lhs, std::string const & rhs, std::unordered_map<std::string, std::unordered_set<std::string>> const & db_s) {
    auto it = db_s.find(lhs);
    if (it != db_s.end())
        return it->second.count(rhs);
    else
        return false;
}

void Data_Structure::BusPolylinesDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & bus_info : buses){
        auto & [bus, color] = bus_info.second;
        auto polyline = Svg::Polyline{}
                .SetStrokeColor(color)
                .SetStrokeWidth(db_svg->renderSettings.line_width)
                .SetStrokeLineJoin("round")
                .SetStrokeLineCap("round");
        for (auto & stop : bus.stops){
            polyline.AddPoint({db_svg->stops_coordinates.at(stop)});
        }
        db_svg->doc.Add(std::move(polyline));
    }
}

void Data_Structure::BusPolylinesDrawer::DrawPartial(const struct DataBaseSvgBuilder * db_svg,
                                                     std::vector<std::string> &names_stops,
                                                     std::vector<std::pair<std::string, size_t>> &used_buses, Svg::Document &doc) {
    size_t i = 0;
    for (auto stop_it = names_stops.begin(); stop_it != std::prev(names_stops.end()); stop_it++, i++) {
        auto &stops = buses.at(used_buses[i].first).first.stops;

        auto beg_it = std::find(stops.begin(), stops.end(), *stop_it);
        auto end_it = std::find(beg_it, stops.end(), *(std::next(stop_it)));
        while (true) {
            if (end_it - beg_it == used_buses[i].second || beg_it == stops.end())
                break;
            else {
                beg_it = std::find(beg_it + 1, stops.end(), *stop_it);
                end_it = std::find(beg_it, stops.end(), *(std::next(stop_it)));
            }
        }
        if (beg_it == stops.end() || end_it == stops.end())
            throw std::logic_error("invalid response on route request!");

        auto polyline = Svg::Polyline{}
                .SetStrokeColor(buses.at(used_buses[i].first).second)
                .SetStrokeWidth(db_svg->renderSettings.line_width)
                .SetStrokeLineJoin("round")
                .SetStrokeLineCap("round");
        for (auto first = beg_it; first != std::next(end_it); first++){
            polyline.AddPoint({db_svg->stops_coordinates.at(*first)});
        }
        doc.Add(std::move(polyline));
    }
}

void Data_Structure::StopsRoundDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [_, coord] : db_svg->stops_coordinates){
        db_svg->doc.Add(Svg::Circle{}
                        .SetFillColor("white")
                        .SetRadius(db_svg->renderSettings.stop_radius)
                        .SetCenter(coord));
    }
}

void
Data_Structure::StopsRoundDrawer::DrawPartial(const struct DataBaseSvgBuilder * db_svg, std::vector<std::string> &names_stops,
                                              std::vector<std::pair<std::string, size_t>> &used_buses,
                                              Svg::Document &doc) {
    size_t i = 0;
    for (auto stop_it = names_stops.begin(); stop_it != std::prev(names_stops.end()); stop_it++, i++) {
        auto &stops = db_svg->bus_dict.at(used_buses[i].first).first.stops;

        auto beg_it = std::find(stops.begin(), stops.end(), *stop_it);
        auto end_it = std::find(beg_it, stops.end(), *(std::next(stop_it)));
        while (true) {
            if (end_it - beg_it == used_buses[i].second || beg_it == stops.end())
                break;
            else {
                beg_it = std::find(beg_it + 1, stops.end(), *stop_it);
                end_it = std::find(beg_it, stops.end(), *(std::next(stop_it)));
            }
        }
        if (beg_it == stops.end() || end_it == stops.end())
            throw std::logic_error("invalid response on route request!");

        for (auto first = beg_it; first != std::next(end_it); first++) {
            doc.Add(Svg::Circle{}
                            .SetFillColor("white")
                            .SetRadius(db_svg->renderSettings.stop_radius)
                            .SetCenter(db_svg->stops_coordinates.at(*first)));
        }
    }
}

void Data_Structure::StopsTextDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [stop_name, coord] : db_svg->stops_coordinates){
        Svg::Text text = Svg::Text{}
                .SetPoint(coord)
                .SetOffset({db_svg->renderSettings.stop_label_offset[0], db_svg->renderSettings.stop_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name);

        Svg::Text substrates = text;
        substrates
                .SetFillColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeWidth(db_svg->renderSettings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round");

        text.SetFillColor("black");

        db_svg->doc.Add(std::move(substrates));
        db_svg->doc.Add(std::move(text));
    }
}

void
Data_Structure::StopsTextDrawer::DrawPartial(const struct DataBaseSvgBuilder * db_svg, std::vector<std::string> &names_stops,
                                             std::vector<std::pair<std::string, size_t>> &used_buses,
                                             Svg::Document &doc) {
    for (auto & stop_name : names_stops){
        Svg::Text text = Svg::Text{}
                .SetPoint(db_svg->stops_coordinates.at(stop_name))
                .SetOffset({db_svg->renderSettings.stop_label_offset[0], db_svg->renderSettings.stop_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name);

        Svg::Text substrates = text;
        substrates
                .SetFillColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeWidth(db_svg->renderSettings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round");

        text.SetFillColor("black");

        doc.Add(std::move(substrates));
        doc.Add(std::move(text));
    }
}

void Data_Structure::BusTextDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [bus_name, bus_info] : buses){
        auto & [bus, color] = bus_info;
        auto beg = bus.stops.begin();
        auto last = std::prev(Ranges::ToMiddle(Ranges::AsRange(bus.stops)).end());

        auto text = Svg::Text{}
                .SetOffset({db_svg->renderSettings.bus_label_offset[0],
                            db_svg->renderSettings.bus_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus.name);

        Svg::Text substrates = text;
        substrates.SetFillColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeWidth(db_svg->renderSettings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round");

        text.SetFillColor(color);

        if (!bus.is_roundtrip && *last != *beg){
            auto text2 = text;
            auto substrates2 = substrates;

            text.SetPoint({db_svg->stops_coordinates.at(*beg)});
            substrates.SetPoint({db_svg->stops_coordinates.at(*beg)});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);

            text2.SetPoint({db_svg->stops_coordinates.at(*last)});
            substrates2.SetPoint({db_svg->stops_coordinates.at(*last)});

            db_svg->doc.Add(substrates2);
            db_svg->doc.Add(text2);
        } else {
            text.SetPoint({db_svg->stops_coordinates.at(*beg)});
            substrates.SetPoint({db_svg->stops_coordinates.at(*beg)});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);
        }
    }
}

void
Data_Structure::BusTextDrawer::DrawPartial(const struct DataBaseSvgBuilder * db_svg, std::vector<std::string> &names_stops,
                                           std::vector<std::pair<std::string, size_t>> &used_buses,
                                           Svg::Document &doc) {
    size_t i = 0;
    for (auto stop_it = names_stops.begin(); stop_it != std::prev(names_stops.end()); stop_it++, i++) {
        auto &bus = db_svg->bus_dict.at(used_buses[i].first).first;

        auto text = Svg::Text{}
                .SetOffset({db_svg->renderSettings.bus_label_offset[0],
                            db_svg->renderSettings.bus_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus.name);

        Svg::Text substrates = text;
        substrates.SetFillColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeWidth(db_svg->renderSettings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round");

        text.SetFillColor(buses.at(used_buses[i].first).second);

        if ((bus.stops.front() == *stop_it) || (!bus.is_roundtrip && *std::prev(Ranges::ToMiddle(Ranges::AsRange(bus.stops)).end()) == *stop_it))
        {
            text.SetPoint({db_svg->stops_coordinates.at(*stop_it)});
            substrates.SetPoint({db_svg->stops_coordinates.at(*stop_it)});

            doc.Add(substrates);
            doc.Add(text);
        }
        if ((bus.stops.front() == *std::next(stop_it)) || (!bus.is_roundtrip && *std::prev(Ranges::ToMiddle(Ranges::AsRange(bus.stops)).end()) == *std::next(stop_it))){
            text.SetPoint({db_svg->stops_coordinates.at(*std::next(stop_it))});
            substrates.SetPoint({db_svg->stops_coordinates.at(*std::next(stop_it))});

            doc.Add(substrates);
            doc.Add(text);
        }
    }
}
