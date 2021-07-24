#include "render_manager.h"
#include "data_manager.h"

#include <cmath>

Data_Structure::MapRespType Data_Structure::DataBaseSvgBuilder::RenderMap() const {
    MapRespType MapResp = std::make_shared<MapResponse>();
    MapResp->svg_xml_answer = doc.Get();
    return MapResp;
}

Data_Structure::DataBaseSvgBuilder::DataBaseSvgBuilder(const Dict<Data_Structure::Stop> &stops,
                                       const Dict<Data_Structure::Bus> &buses,
                                       RenderSettings render_set) : renderSettings(std::move(render_set)){
    Init(stops, buses);
    CalculateCoordinates(stops, buses);
    for (const auto & layer : renderSettings.layers) {
        (layersStrategy[layer])->Draw(this);
    }

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
                             * (k - l);
                double y = left_bearing_distance.GetLatitude()
                           + step(left_bearing_distance.GetLatitude(),
                                  right_bearing_distance.GetLatitude(), count)
                             * (k - l);
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

// TODO придумать более оптимальное решение чем паттерн стратегия
void Data_Structure::DataBaseSvgBuilder::Init(const Dict<Data_Structure::Stop> &stops,
                                              const Dict<Data_Structure::Bus> &buses) {
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("bus_lines"),
                           std::forward_as_tuple(std::make_shared<BusPolylinesDrawer>(stops, buses)));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("bus_labels"),
                           std::forward_as_tuple(std::make_shared<BusTextDrawer>(stops, buses)));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("stop_points"),
                           std::forward_as_tuple(std::make_shared<StopsRoundDrawer>(stops)));
    layersStrategy.emplace(std::piecewise_construct,
                           std::forward_as_tuple("stop_labels"),
                           std::forward_as_tuple(std::make_shared<StopsTextDrawer>(stops)));

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
    size_t size = db_svg->renderSettings.color_palette.size();
    size_t i = 0;

    for (auto & bus : buses){
        auto polyline = Svg::Polyline{}
                .SetStrokeColor(db_svg->renderSettings.color_palette[i++ % size])
                .SetStrokeWidth(db_svg->renderSettings.line_width)
                .SetStrokeLineJoin("round")
                .SetStrokeLineCap("round");
        for (auto & stop : bus.second->stops){
            auto & distance = stops.at(stop)->dist;
            polyline.AddPoint({db_svg->stops_coordinates.at(stops.at(stop)->name)});
        }
        db_svg->doc.Add(std::move(polyline));
    }
}

void Data_Structure::StopsRoundDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [_, stop] : stops){
        auto & distance = stop->dist;
        db_svg->doc.Add(Svg::Circle{}
                        .SetFillColor("white")
                        .SetRadius(db_svg->renderSettings.stop_radius)
                        .SetCenter(db_svg->stops_coordinates.at(stop->name)));
    }
}

void Data_Structure::StopsTextDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [_, stop] : stops){
        auto & distance = stop->dist;
        auto & name = stop->name;
        Svg::Text text = Svg::Text{}
                .SetPoint({db_svg->stops_coordinates.at(name)})
                .SetOffset({db_svg->renderSettings.stop_label_offset[0], db_svg->renderSettings.stop_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(name);

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

void Data_Structure::BusTextDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    size_t size = db_svg->renderSettings.color_palette.size();
    size_t i = 0;
    for (auto & [bus_name, bus] : buses){
        auto beg = bus->stops.begin();
        auto last = std::prev(Ranges::ToMiddle(Ranges::AsRange(bus->stops)).end());

        auto text = Svg::Text{}
                .SetOffset({db_svg->renderSettings.bus_label_offset[0],
                            db_svg->renderSettings.bus_label_offset[1]})
                .SetFontSize(db_svg->renderSettings.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->name);

        Svg::Text substrates = text;
        substrates.SetFillColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeColor(db_svg->renderSettings.underlayer_color)
                .SetStrokeWidth(db_svg->renderSettings.underlayer_width)
                .SetStrokeLineCap("round")
                .SetStrokeLineJoin("round");

        text.SetFillColor(db_svg->renderSettings.color_palette[i++ % size]);

        if (!bus->is_roundtrip && *last != *beg){
            auto text2 = text;
            auto substrates2 = substrates;

            text.SetPoint({db_svg->stops_coordinates.at(stops.at(*beg)->name)});
            substrates.SetPoint({db_svg->stops_coordinates.at(stops.at(*beg)->name)});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);

            text2.SetPoint({db_svg->stops_coordinates.at(stops.at(*last)->name)});
            substrates2.SetPoint({db_svg->stops_coordinates.at(stops.at(*last)->name)});

            db_svg->doc.Add(substrates2);
            db_svg->doc.Add(text2);
        } else {
            text.SetPoint({db_svg->stops_coordinates.at(stops.at(*beg)->name)});
            substrates.SetPoint({db_svg->stops_coordinates.at(stops.at(*beg)->name)});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);
        }
    }
}
