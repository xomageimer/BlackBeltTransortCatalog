#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <map>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

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
        double outer_margin;
        double padding;
        double stop_radius;
        double line_width;

        int stop_label_font_size;
        double stop_label_offset[2] {0};

        int bus_label_font_size;
        double bus_label_offset[2] {0};

        Svg::Color underlayer_color;
        double underlayer_width;

        std::vector<Svg::Color> color_palette;

        std::vector<std::string> layers;
    };

    struct ILayersStrategy {
        explicit ILayersStrategy(struct DataBaseSvgBuilder * myself_svg_builder) : db_svg(myself_svg_builder) {}
        virtual void Draw() = 0;
        virtual void DrawPartial(std::vector<std::string>& names_stops, std::vector<std::pair<std::string, size_t>> & used_buses, Svg::Document & doc) const = 0;
    protected:
        struct DataBaseSvgBuilder * db_svg;
    };

    struct BusPolylinesDrawer : public ILayersStrategy{
    public:
        using ILayersStrategy::ILayersStrategy;
        void Draw() override;
        void DrawPartial(std::vector<std::string>& names_stops, std::vector<std::pair<std::string, size_t>> & used_buses, Svg::Document & doc) const override;
    };

    struct StopsRoundDrawer : public ILayersStrategy{
    public:
        using ILayersStrategy::ILayersStrategy;
        void Draw() override;
        void DrawPartial(std::vector<std::string>& names_stops, std::vector<std::pair<std::string, size_t>> & used_buses, Svg::Document & doc) const override;
    private:
        void RenderRoundLabel(Svg::Document& doc, const std::string & stop_name) const;
    };

    struct StopsTextDrawer : public ILayersStrategy{
    public:
        using ILayersStrategy::ILayersStrategy;
        void Draw() override;
        void DrawPartial(std::vector<std::string>& names_stops, std::vector<std::pair<std::string, size_t>> & used_buses, Svg::Document & doc) const override;
    private:
        void RenderStopLabel(Svg::Document& doc, const std::string& stop_name) const;
    };

    struct BusTextDrawer : public ILayersStrategy{
    public:
        using ILayersStrategy::ILayersStrategy;
        void Draw() override;
        void DrawPartial(std::vector<std::string>& names_stops, std::vector<std::pair<std::string, size_t>> & used_buses, Svg::Document & doc) const override;
    private:
        void RenderBusLabel(Svg::Document& doc, const std::string& bus_name, const std::string& stop_name) const;
    };

    struct DataBaseSvgBuilder {
    public:
        explicit DataBaseSvgBuilder(const Dict<struct Stop> &stops, const Dict<struct Bus> &buses,
                                    RenderSettings render_set);
        [[nodiscard]] MapRespType RenderMap() const;
        [[nodiscard]] MapRespType RenderRoute(std::vector<RouteResponse::ItemPtr> const &);
        static bool IsConnected(std::string const & lhs, std::string const & rhs, std::unordered_map<std::string, std::unordered_set<std::string>> const & db_s);

        friend BusPolylinesDrawer;
        friend StopsRoundDrawer;
        friend StopsTextDrawer;
        friend BusTextDrawer;
    private:
        void Init(const Dict<struct Bus> &buses);
        void CalculateCoordinates(const Dict<struct Stop> & stops, const Dict<struct Bus> &buses);

        std::map<std::string, Svg::Point> CoordinateUniformDistribution(const Dict<struct Stop> &stops, const Dict<struct Bus> & buses);
        void BuildNeighborhoodConnections( std::map<std::string, Svg::Point> const & new_coords, const Dict<struct Bus> &buses);
        static auto SortingByCoordinates(std::map<std::string, Svg::Point> const & uniform, const Dict<struct Stop> &stops);
        std::pair<std::map<std::string, int>, int> GluingCoordinates(std::vector<std::pair<double, std::string>> const & sorted_by_coord);
        std::map<std::string, Svg::Point> CoordinateCompression(const Dict<struct Stop> &stops, const Dict<struct Bus> & buses);

        RenderSettings renderSettings;
        Svg::Document doc;

        std::map<std::string, std::pair<Bus, Svg::Color>> bus_dict;
        std::map<std::string, Svg::Point> stops_coordinates;
        std::map<std::string, std::shared_ptr<ILayersStrategy>> layersStrategy;

        std::unordered_map<std::string, std::unordered_set<std::string>> db_connected;
    };
}

#endif //RENDER_MANAGER_H
