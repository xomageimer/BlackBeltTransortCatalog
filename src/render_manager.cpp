#include "render_manager.h"
#include "data_manager.h"

#include<cmath>

Data_Structure::MapRespType Data_Structure::DataBaseSvgBuilder::RenderMap() const {
    MapRespType MapResp = std::make_shared<MapResponse>();
    MapResp->svg_xml_answer = doc.Get();
    return MapResp;
}

Data_Structure::DataBaseSvgBuilder::DataBaseSvgBuilder(const Dict<Data_Structure::Stop> &stops,
                                       const Dict<Data_Structure::Bus> &buses,
                                       RenderSettings render_set) : renderSettings(std::move(render_set)){
    Init(stops, buses);
    CalculateCoordinates(stops);
    for (const auto & layer : renderSettings.layers) {
        (layersStrategy[layer])->Draw(this);
    }

    doc.SimpleRender();
}

auto Data_Structure::DataBaseSvgBuilder::CoordinateCompression(const Dict<Data_Structure::Stop> &stops) {
    std::map<double, std::string> sorted_x;
    std::map<double, std::string, std::greater<>> sorted_y;

    for (auto & [_, stop] : stops) {
        auto distance = stop->dist;
        sorted_x.emplace(distance.GetLongitude(), stop->name);
        sorted_y.emplace(distance.GetLatitude(), stop->name);
    }

    int point_size = sorted_x.size() - 1;
    std::map<std::string, double> new_x;
    std::map<std::string, double> new_y;
    if (point_size == 0){
        new_x.emplace(stops.begin()->first, renderSettings.padding);
        new_y.emplace(stops.begin()->first, renderSettings.height - renderSettings.padding);
    } else {
        double x_step = (renderSettings.width - 2. * renderSettings.padding) / point_size;
        double y_step = (renderSettings.height - 2. * renderSettings.padding) / point_size;

        size_t i = 0;
        for (auto &el : sorted_x) {
            new_x.emplace(el.second, i * x_step + renderSettings.padding);
            i++;
        }
        i = 0;
        for (auto &el : sorted_y) {
            new_y.emplace(el.second, renderSettings.height - renderSettings.padding - i * y_step);
            i++;
        }
    }

    std::map<std::string, Distance> CompressCoord;

    for (auto & [stop_name, _] : stops){
        CompressCoord.emplace(stop_name, Distance{new_x.at(stop_name), new_y.at(stop_name)});
    }
    return CompressCoord;
}

void Data_Structure::DataBaseSvgBuilder::CalculateCoordinates(const Dict<Data_Structure::Stop> &stops) {
    auto compress_stops = CoordinateCompression(stops);

    if (stops.size() == 1){
        auto stop = stops.begin()->second;
        auto & dist = compress_stops.begin()->second;
        stops_coordinates.emplace(stop->name, Svg::Point{dist.GetLongitude(), dist.GetLatitude()});
        return;
    }

    std::pair<std::optional<double>, std::optional<double>> min_lon_lat;
    std::pair<std::optional<double>, std::optional<double>> max_lon_lat;
    for (auto & [stop_name, dist] : compress_stops){
        auto lon = std::abs(dist.GetLongitude());
        auto lat = std::abs(dist.GetLatitude());

        if (!min_lon_lat.first || min_lon_lat.first > lon)
            min_lon_lat.first = lon;
        if (!min_lon_lat.second || min_lon_lat.second > lat)
            min_lon_lat.second = lat;
        if (!max_lon_lat.first || max_lon_lat.first < lon)
            max_lon_lat.first = lon;
        if (!max_lon_lat.second || max_lon_lat.second < lat)
            max_lon_lat.second = lat;
    }
    auto min_lon = min_lon_lat.first.value_or(0);
    auto max_lat = max_lon_lat.second.value_or(0);

    double width_zoom_coef = 0;
    double height_zoom_coef = 0;
    if (max_lon_lat.first.value() - min_lon_lat.first.value() != 0)
        width_zoom_coef = (renderSettings.width - 2 * renderSettings.padding) /
                (max_lon_lat.first.value() - min_lon_lat.first.value());
    if (max_lon_lat.second.value() - min_lon_lat.second.value() != 0)
        height_zoom_coef = (renderSettings.height - 2 * renderSettings.padding) /
                (max_lon_lat.second.value() - min_lon_lat.second.value());

    auto min_ = [](const auto first, const auto sec) {
        double min = 0.;
        if (first != 0 && sec != 0)
            min = std::min(first, sec);
        else if (first != 0)
            min = first;
        else if (sec != 0) min = sec;
        return min;
    };
    auto zoom_coef = min_(width_zoom_coef, height_zoom_coef);

    auto call_x = [min_lon = min_lon, zoom_coef = zoom_coef, padding = this->renderSettings.padding](double lon) {
        return (std::abs(lon) - min_lon) * zoom_coef + padding;
    };
    auto call_y = [max_lat = max_lat, zoom_coef = zoom_coef, padding = this->renderSettings.padding](double lat) {
        return (max_lat - std::abs(lat)) * zoom_coef + padding;
    };

    for (auto & [stop_name, dist] : compress_stops){
        stops_coordinates.emplace(stop_name, Svg::Point{call_x(dist.GetLongitude()), call_y(dist.GetLatitude())});
    }
}

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
    for (auto & [_, bus] : buses){
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
