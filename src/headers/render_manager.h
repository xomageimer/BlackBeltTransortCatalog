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
        virtual void Draw(struct DataBaseSvgBuilder *) = 0;
    };

    struct BusPolylinesDrawer : public ILayersStrategy{
    public:
        explicit BusPolylinesDrawer(const Dict<struct Stop> & stp_, const Dict<struct Bus> & bus_) : stops(stp_), buses(bus_) {}
        void Draw(struct DataBaseSvgBuilder *) override;
    private:
        const Dict<struct Bus> & buses;
        const Dict<struct Stop> & stops;
    };

    struct StopsRoundDrawer : public ILayersStrategy{
    public:
        explicit StopsRoundDrawer(const Dict<Data_Structure::Stop> & stp_) : stops(stp_) {}
        void Draw(struct DataBaseSvgBuilder *) override;
    private:
        const Dict<struct Stop> & stops;
    };

    struct StopsTextDrawer : public ILayersStrategy{
    public:
        explicit StopsTextDrawer(const Dict<Data_Structure::Stop> & stp_) : stops(stp_) {}
        void Draw(struct DataBaseSvgBuilder *) override;
    private:
        const Dict<struct Stop> & stops;
    };

    struct BusTextDrawer : public ILayersStrategy{
    public:
        explicit BusTextDrawer(const Dict<Data_Structure::Stop> & stps_, const Dict<Data_Structure::Bus> & bus_) : stops(stps_),  buses(bus_) {}
        void Draw(struct DataBaseSvgBuilder *) override;
    private:
        const Dict<struct Bus> & buses;
        const Dict<struct Stop> & stops;
    };

    struct DataBaseSvgBuilder {
    public:
        explicit DataBaseSvgBuilder(const Dict<struct Stop> &stops, const Dict<struct Bus> &buses,
                                    RenderSettings render_set);
        [[nodiscard]] MapRespType RenderMap() const;
        static bool IsConnected(std::string const & lhs, std::string const & rhs, std::unordered_map<std::string, std::unordered_set<std::string>> const & db_s);

        friend BusPolylinesDrawer;
        friend StopsRoundDrawer;
        friend StopsTextDrawer;
        friend BusTextDrawer;
    private:
        void Init(const Dict<struct Stop> &stops, const Dict<struct Bus> &buses);
        void CalculateCoordinates(const Dict<struct Stop> & stops, const Dict<struct Bus> &buses);

        std::map<std::string, Svg::Point> CoordinateUniformDistribution(const Dict<struct Stop> &stops, const Dict<struct Bus> & buses);
        void BuildNeighborhoodConnections( std::map<std::string, Svg::Point> const & new_coords, const Dict<struct Bus> &buses);
        auto SortingByCoordinates(std::map<std::string, Svg::Point> const & uniform, const Dict<struct Stop> &stops);
        std::pair<std::map<std::string, int>, int> GluingCoordinates(std::vector<std::pair<double, std::string>> const & sorted_by_coord);
        std::map<std::string, Svg::Point> CoordinateCompression(const Dict<struct Stop> &stops, const Dict<struct Bus> & buses);

        RenderSettings renderSettings;
        Svg::Document doc;

        std::map<std::string, Svg::Point> stops_coordinates;

        std::map<std::string, std::shared_ptr<ILayersStrategy>> layersStrategy;

        std::unordered_map<std::string, std::unordered_set<std::string>> db_connected;
    };
}

#endif //RENDER_MANAGER_H
