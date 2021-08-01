#ifndef GRAPH_H
#define GRAPH_H

#include "ranges.h"
#include <vector>

// TODO сериализовывать/десериализовывать граф тоже

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
        explicit DirectedWeightedGraph(const Serialize::Graph &);

        EdgeID AddEdge(Edge<Weight> const &);

        [[nodiscard]] size_t GetEdgeCount() const;
        [[nodiscard]] size_t GetVertexCount() const;

        Edge<Weight> const & GetEdge(EdgeID) const;
        [[nodiscard]] IncidenceEdgesRange GetIncidenceList(VertexID) const;

        void Serialize(Serialize::Graph & graph_mes) const;
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
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(const Serialize::Graph & graph_mes) {
        for (auto & edge : graph_mes.edges()){
            this->edges.emplace_back(Edge<Weight>{edge.from(), edge.to(), edge.weight()});
        }
    }

    template<typename Weight>
    void DirectedWeightedGraph<Weight>::Serialize(Serialize::Graph &graph_mes) const {
        for (auto & edge : edges) {
            Serialize::EdgeData ed;
            ed.set_from(edge.from);
            ed.set_to(edge.to);
            ed.set_weight(edge.weight);

            *graph_mes.add_edges() = std::move(ed);
        }
    }
}

#endif //GRAPH_H