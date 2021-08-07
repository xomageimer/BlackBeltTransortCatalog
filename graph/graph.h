#ifndef GRAPH_H
#define GRAPH_H

#include "ranges.h"
#include <vector>

#include "transport_catalog.pb.h"

namespace Graph {
    using VertexID = size_t;
    using EdgeID = size_t;

    template <typename Weight>
    struct Edge{
        VertexID from;
        VertexID to;
        Weight weight;
    };

    template <typename Weight>
    struct DirectedWeightedGraph{
        using IncidenceList = std::vector<EdgeID>;
        using EdgeList = std::vector<Edge<Weight>>;
        using IncidenceEdgesRange = Ranges::Range<typename IncidenceList::const_iterator>;

        explicit DirectedWeightedGraph(size_t VertexCount) : incidence_lists(VertexCount) {}
        explicit DirectedWeightedGraph(const Serialize::Router &router_mes);

        EdgeID AddEdge(Edge<Weight> const &);

        [[nodiscard]] size_t GetEdgeCount() const;
        [[nodiscard]] size_t GetVertexCount() const;

        Edge<Weight> const & GetEdge(EdgeID) const;
        [[nodiscard]] IncidenceEdgesRange GetIncidenceList(VertexID) const;

        void Serialize(Serialize::Router &router_mes) const;
    private:
        EdgeList edges;
        std::vector<IncidenceList> incidence_lists;
    };

    template<typename Weight>
    EdgeID DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight> & edge) {
        edges.push_back(edge);
        const EdgeID id = edges.size() - 1;
        incidence_lists[edge.from].push_back(id);
        return id;
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges.size();
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists.size();
    }

    template<typename Weight>
    const Edge<Weight> &DirectedWeightedGraph<Weight>::GetEdge(EdgeID edge_id) const {
        return edges[edge_id];
    }

    template<typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidenceEdgesRange DirectedWeightedGraph<Weight>::GetIncidenceList(VertexID vertex_id) const {
        const auto & es = incidence_lists[vertex_id];
        return {es.begin(), es.end()};
    }

    template<typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(const Serialize::Router & router_mes) : incidence_lists(router_mes.vertexes().size() * 2) {
        for (auto & edge : router_mes.edges()){
            this->edges.emplace_back(Edge<Weight>{edge.vert_id_from(), edge.vert_id_to(), edge.weight()});
            this->incidence_lists[edge.vert_id_from()].push_back(edge.id());
        }
    }

    template<typename Weight>
    void DirectedWeightedGraph<Weight>::Serialize(Serialize::Router &router_mes) const {
        size_t i = 0;
        for (auto & edge : edges){
            Serialize::Edge ser_edge;

            ser_edge.set_vert_id_from(edge.from);
            ser_edge.set_vert_id_to(edge.to);
            ser_edge.set_weight(edge.weight);
            ser_edge.set_id(i++);
            *router_mes.add_edges() = std::move(ser_edge);
        }
    }
}

#endif //GRAPH_H