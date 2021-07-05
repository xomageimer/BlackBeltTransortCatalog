#include "render_manager.h"
#include "data_manager.h"

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

void Data_Structure::DataBaseSvgBuilder::CalculateCoordinates(const Dict<Data_Structure::Stop> &stops) {
    std::pair<std::optional<double>, std::optional<double>> min_lon_lat;
    std::pair<std::optional<double>, std::optional<double>> max_lon_lat;
    for (auto & [_, stop] : stops){
        auto lon = std::abs(stop->dist.GetLongitude());
        auto lat = std::abs(stop->dist.GetLatitude());

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

    cal_x = [min_lon = min_lon, zoom_coef = zoom_coef, padding = this->renderSettings.padding](double lon) {
        return (std::abs(lon) - min_lon) * zoom_coef + padding;
    };
    cal_y = [max_lat = max_lat, zoom_coef = zoom_coef, padding = this->renderSettings.padding](double lat) {
        return (max_lat - std::abs(lat)) * zoom_coef + padding;
    };
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
            polyline.AddPoint({db_svg->cal_x(distance.GetLongitude()), db_svg->cal_y(distance.GetLatitude())});
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
                        .SetCenter({db_svg->cal_x(distance.GetLongitude()), db_svg->cal_y(distance.GetLatitude())}));
    }
}

void Data_Structure::StopsTextDrawer::Draw(struct DataBaseSvgBuilder * db_svg) {
    for (auto & [_, stop] : stops){
        auto & distance = stop->dist;
        auto & name = stop->name;
        Svg::Text text = Svg::Text{}
                .SetPoint({db_svg->cal_x(distance.GetLongitude()), db_svg->cal_y(distance.GetLatitude())})
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

            text.SetPoint({db_svg->cal_x(stops.at(*beg)->dist.GetLongitude()),
                           db_svg->cal_y( stops.at(*beg)->dist.GetLatitude())});
            substrates.SetPoint({db_svg->cal_x(stops.at(*beg)->dist.GetLongitude()),
                                 db_svg->cal_y(stops.at(*beg)->dist.GetLatitude())});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);

            text2.SetPoint({db_svg->cal_x(stops.at(*last)->dist.GetLongitude()),
                            db_svg->cal_y(stops.at(*last)->dist.GetLatitude())});
            substrates2.SetPoint({db_svg->cal_x(stops.at(*last)->dist.GetLongitude()),
                                  db_svg->cal_y(stops.at(*last)->dist.GetLatitude())});

            db_svg->doc.Add(substrates2);
            db_svg->doc.Add(text2);
        } else {
            text.SetPoint({db_svg->cal_x(stops.at(*beg)->dist.GetLongitude()),
                           db_svg->cal_y(stops.at(*beg)->dist.GetLatitude())});
            substrates.SetPoint({db_svg->cal_x(stops.at(*beg)->dist.GetLongitude()),
                                 db_svg->cal_y(stops.at(*beg)->dist.GetLatitude())});

            db_svg->doc.Add(substrates);
            db_svg->doc.Add(text);
        }
    }
}
