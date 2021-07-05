#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <unordered_map>
#include <functional>

#include "responses.h"

#include "xml.h"
#include "svg.h"

template <typename T>
using Dict = std::map<std::string, const T *>;

namespace Data_Structure {
    using MapRespType = std::shared_ptr<MapResponse>;
    struct RenderSettings {
        double width;
        double height;
        double padding;
        double stop_radius;
        double line_width;

        int stop_label_font_size;
        double stop_label_offset[2];

        int bus_label_font_size;
        double bus_label_offset[2];

        Svg::Color underlayer_color;
        double underlayer_width;

        std::vector<Svg::Color> color_palette;
    };

    struct DataBaseSvgBuilder {
    public:
        using RouteRespType = std::shared_ptr<RouteResponse>;

        explicit DataBaseSvgBuilder(const Dict<struct Stop> &stops, const Dict<struct Bus> &buses,
                                    RenderSettings render_set);
        [[nodiscard]] MapRespType RenderMap() const;

    private:
        void CalculateCoordinates(const Dict<struct Stop> & stops);
        void DrawStopsPolylines(const Dict<struct Bus> &, const Dict<Data_Structure::Stop> &);
        void DrawStopsRound(const Dict<Data_Structure::Stop> &);
        void DrawStopsText(const Dict<Data_Structure::Stop> &);
        void DrawBusText(const Dict<Data_Structure::Bus> & buses, const Dict<Data_Structure::Stop> & stops);

        std::vector<std::pair<double, double>> stops_coordinates;
        RenderSettings renderSettings;
        Svg::Document doc;

        std::function<double(double)> cal_x;
        std::function<double(double)> cal_y;
    };
}

#endif //RENDER_MANAGER_H
