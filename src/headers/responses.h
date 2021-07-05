#ifndef RESPONSES_H
#define RESPONSES_H

#include <set>
#include <string_view>
#include <memory>

#include "xml.h"
#include "json.h"

using JsonResponse = Json::Node;
using ResponseType = std::shared_ptr<struct Response>;

struct Response {
public:
    using Dict = std::map<std::string, Json::Node>;
    int id;
    virtual void MakeJson() = 0;

    [[nodiscard]] Dict const & GetJson() const;
protected:
    Dict valid_data;
};

struct StopResponse : public Response {
public:
    void MakeJson() override;

    std::set<std::string> buses;
};

struct BusResponse : public Response {
public:
    void MakeJson() override;

    int length{};
    double curvature{};
    size_t stop_count{};
    size_t unique_stop_count{};
};

struct RouteResponse : public Response {
public:
    struct Item {
        enum class ItemType {
            WAIT,
            BUS
        };
        explicit Item(ItemType t) : type(t) {}
        const ItemType type;
        std::string name {};
        double time {};
    };
    struct Wait : public Item {
        Wait() : Item(ItemType::WAIT) {};
    };
    struct Bus : public Item {
        explicit Bus(size_t span_c) : Item(ItemType::BUS), span_count(span_c) {};
        size_t span_count{};
    };
    using ItemPtr = std::shared_ptr<Item>;

    void MakeJson() override;

    double total_time;
    std::vector<ItemPtr> items;
};

struct MapResponse : public Response {
public:
    void MakeJson() override;

    XML::xml svg_xml_answer;
};

struct BadResponse : public Response {
    void MakeJson() override;

    std::string error_message;
};

void PrintResponses(std::vector<JsonResponse>, std::ostream & os = std::cout);

#endif //RESPONSES_H
