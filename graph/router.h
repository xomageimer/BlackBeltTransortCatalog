#ifndef ROUTER_H
#define ROUTER_H

#include "graph.h"

#include <set>
#include <cassert>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <optional>

namespace Graph {
    template <typename Weight>
    struct Router{
    private:
        using Graph_t = DirectedWeightedGraph<Weight>;
        using RouteID = uint64_t;
    public:
        explicit Router(const Graph_t &);

        struct RouteInfo {
            RouteID id;
            Weight weight;
            size_t edge_count;
        };

        std::optional<RouteInfo> BuildRoute(VertexID from, VertexID to) const;
        [[nodiscard]] EdgeID GetRouteEdge(RouteID route_id, size_t edge_idx) const;
        auto GetRouteRangeOfEdges(RouteID route_id) const;
        void ReleaseRoute(RouteID route_id);
        bool CheckCache(RouteID route_id);
    private:
        struct RouteInternalData {
            Weight weight;
            VertexID vertex_number{};
            std::optional<EdgeID> prev_edge;

            bool operator<(RouteInternalData const & rhs) const{
                return weight < rhs.weight || (weight == rhs.weight && vertex_number < rhs.vertex_number);
            }
        };
        using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

        Graph_t const & graph;
        RoutesInternalData routes_internal_data;

        mutable std::unordered_map<RouteID, std::vector<EdgeID>> routes_cache;
        mutable size_t routes_index {0};

        void InitializeRouteInternalData(){
            const size_t vertex_count = graph.GetVertexCount();
            for (size_t vertex_id = 0; vertex_id < vertex_count; vertex_id++){
                routes_internal_data[vertex_id][vertex_id] = RouteInternalData{0, vertex_id, std::nullopt};
            }
        }

        void RelaxRoute(VertexID vertex_id) {
            auto &relax_route = routes_internal_data[vertex_id];
            std::set<RouteInternalData> heap_of_route_internal_data;
            heap_of_route_internal_data.insert(*relax_route[vertex_id]);
            while (!heap_of_route_internal_data.empty()) {
                auto min_vert = *heap_of_route_internal_data.begin();
                heap_of_route_internal_data.erase(heap_of_route_internal_data.begin());

                for (EdgeID edge_id : graph.GetIncidenceList(min_vert.vertex_number)) {
                    auto const & edge = graph.GetEdge(edge_id);
                    auto len = edge.weight;

                    if (!relax_route[edge.to] || min_vert.weight + len < relax_route[edge.to]->weight) {
                        auto rt = RouteInternalData{min_vert.weight + len, edge.to, edge_id};
                        heap_of_route_internal_data.erase(rt);
                        relax_route[edge.to] = rt;
                        heap_of_route_internal_data.insert(rt);
                    }
                }
            }
        }
    };

    template<typename Weight>
    Router<Weight>::Router(const Router::Graph_t & graph_ref) :
        graph(graph_ref),
        routes_internal_data(graph.GetVertexCount(), std::vector<std::optional<RouteInternalData>>(graph.GetVertexCount()))
    {
        InitializeRouteInternalData();

        for (size_t i = 0; i < graph.GetVertexCount(); i++){
            RelaxRoute(i);
        }
    }

    template<typename Weight>
    std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexID from, VertexID to) const {
        auto const & router = routes_internal_data[from];
        auto finish = router[to];
        if (finish){
            std::vector<EdgeID> edges;
            for (std::optional<EdgeID> edge = finish->prev_edge;
                 edge; finish = router[graph.GetEdge(*edge).from], edge = finish->prev_edge){
                edges.push_back(*edge);
            }
            RouteID new_route_id = routes_index++;
            routes_cache[new_route_id] = std::move(edges);
            return RouteInfo{new_route_id, router[to]->weight, routes_cache[new_route_id].size()};
        } else {
            return std::nullopt;
        }
    }

    template<typename Weight>
    EdgeID Router<Weight>::GetRouteEdge(Router::RouteID route_id, size_t edge_idx) const {
        return routes_cache.at(route_id)[edge_idx];
    }

    template<typename Weight>
    void Router<Weight>::ReleaseRoute(Router::RouteID route_id) {
        routes_cache.erase(route_id);
    }

    template<typename Weight>
    auto Router<Weight>::GetRouteRangeOfEdges(Router::RouteID route_id) const {
        return Ranges::Reverse(Ranges::AsRange<std::vector<EdgeID>>(routes_cache.at(route_id)));
    }

    template<typename Weight>
    bool Router<Weight>::CheckCache(Router::RouteID route_id) {
        return std::end(routes_cache) != routes_cache.find(route_id);
    }
}

#endif //ROUTER_H