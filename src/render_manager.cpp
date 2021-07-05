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
    CalculateCoordinates(stops);
    DrawStopsPolylines(buses, stops);
    DrawStopsRound(stops);
    DrawStopsText(stops);
    DrawBusText(buses, stops);

    doc.SimpleRender();
}

void Data_Structure::DataBaseSvgBuilder::DrawStopsPolylines(const Dict<Data_Structure::Bus> & buses, const Dict<Data_Structure::Stop> &stops) {
    size_t size = renderSettings.color_palette.size();
    size_t i = 0;

    for (auto & bus : buses){
        auto polyline = Svg::Polyline{}
                    .SetStrokeColor(renderSettings.color_palette[i++ % size])
                    .SetStrokeWidth(renderSettings.line_width)
                    .SetStrokeLineJoin("round")
                    .SetStrokeLineCap("round");
        for (auto & stop : bus.second->stops){
            auto & distance = stops.at(stop)->dist;
            polyline.AddPoint({cal_x(distance.GetLongitude()), cal_y(distance.GetLatitude())});
        }
        doc.Add(std::move(polyline));
    }
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

void Data_Structure::DataBaseSvgBuilder::DrawStopsRound(const Dict<Data_Structure::Stop> & stops) {
    for (auto & [_, stop] : stops){
        auto & distance = stop->dist;
        doc.Add(Svg::Circle{}
                    .SetFillColor("white")
                    .SetRadius(renderSettings.stop_radius)
                    .SetCenter({cal_x(distance.GetLongitude()), cal_y(distance.GetLatitude())}));
    }
}

void Data_Structure::DataBaseSvgBuilder::DrawStopsText(const Dict<Data_Structure::Stop> & stops) {
    for (auto & [_, stop] : stops){
        auto & distance = stop->dist;
        auto & name = stop->name;
        Svg::Text text = Svg::Text{}
                        .SetPoint({cal_x(distance.GetLongitude()), cal_y(distance.GetLatitude())})
                        .SetOffset({renderSettings.stop_label_offset[0], renderSettings.stop_label_offset[1]})
                        .SetFontSize(renderSettings.stop_label_font_size)
                        .SetFontFamily("Verdana")
                        .SetData(name);

        Svg::Text substrates = text;
        substrates
        .SetFillColor(renderSettings.underlayer_color)
        .SetStrokeColor(renderSettings.underlayer_color)
        .SetStrokeWidth(renderSettings.underlayer_width)
        .SetStrokeLineCap("round")
        .SetStrokeLineJoin("round");

        text.SetFillColor("black");

        doc.Add(std::move(substrates));
        doc.Add(std::move(text));
    }
}

void Data_Structure::DataBaseSvgBuilder::DrawBusText(const Dict<Data_Structure::Bus> &buses,
                                                     const Dict<Data_Structure::Stop> &stops) {
    size_t size = renderSettings.color_palette.size();
    size_t i = 0;
    for (auto & [_, bus] : buses){
        auto beg = bus->stops.begin();
        auto last = std::prev(bus->stops.end());

        auto text = Svg::Text{}
                    .SetOffset({renderSettings.bus_label_offset[0],
                                renderSettings.bus_label_offset[1]})
                    .SetFontSize(renderSettings.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus->name);
        Svg::Text substrates = text;
        substrates.SetFillColor(renderSettings.underlayer_color)
                  .SetStrokeColor(renderSettings.underlayer_color)
                  .SetStrokeWidth(renderSettings.underlayer_width)
                  .SetStrokeLineCap("round")
                  .SetStrokeLineJoin("round");
        text.SetFillColor(renderSettings.color_palette[i++ % size]);

        if (!bus->is_roundtrip && *last != *beg){
            auto text2 = text;
            auto substrates2 = substrates;

            text.SetPoint({stops.at(*beg)->dist.GetLongitude(),
                           stops.at(*beg)->dist.GetLatitude()});
            substrates.SetPoint({stops.at(*beg)->dist.GetLongitude(),
                                 stops.at(*beg)->dist.GetLatitude()});

            doc.Add(text);
            doc.Add(substrates);

            text2.SetPoint({stops.at(*last)->dist.GetLongitude(),
                            stops.at(*last)->dist.GetLatitude()});
            substrates2.SetPoint({stops.at(*last)->dist.GetLongitude(),
                                 stops.at(*last)->dist.GetLatitude()});

            doc.Add(text2);
            doc.Add(substrates2);
        } else {
            auto text2 = text;
            auto substrates2 = substrates;

            text.SetPoint({stops.at(*beg)->dist.GetLongitude(),
                           stops.at(*beg)->dist.GetLatitude()});
            substrates.SetPoint({stops.at(*beg)->dist.GetLongitude(),
                                 stops.at(*beg)->dist.GetLatitude()});

            doc.Add(text);
            doc.Add(substrates);
        }
    }
}
