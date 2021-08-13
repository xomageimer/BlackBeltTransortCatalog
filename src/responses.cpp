#include "responses.h"

#include "company.pb.h"

Response::Dict const & Response::GetJson() const {
    return valid_data;
}

void StopResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    std::vector<Json::Node> buses_;
    for (const auto & el : buses){
        buses_.emplace_back(el);
    }
    valid_data.emplace("buses", buses_);
}

void BusResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("route_length"), std::forward_as_tuple(length));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("curvature"), std::forward_as_tuple(curvature));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("stop_count"), std::forward_as_tuple(static_cast<int>(stop_count)));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("unique_stop_count"), std::forward_as_tuple(static_cast<int>(unique_stop_count)));
}

void RouteResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("total_time"), std::forward_as_tuple(total_time));
    std::vector<Json::Node> items_;
    for (auto & el : items){
        Dict item;
        item.emplace(std::piecewise_construct, std::forward_as_tuple("time"), std::forward_as_tuple(static_cast<double>(el->time)));
        if (el->type == Item::ItemType::BUS){
            item.emplace(std::piecewise_construct, std::forward_as_tuple("bus"), std::forward_as_tuple(el->name));
            item.emplace(std::piecewise_construct, std::forward_as_tuple("type"), std::forward_as_tuple(std::string("Bus")));
            item.emplace(std::piecewise_construct, std::forward_as_tuple("span_count"), std::forward_as_tuple(static_cast<int>(reinterpret_cast<Bus*>(el.get())->span_count)));
        } else {
            item.emplace(std::piecewise_construct, std::forward_as_tuple("stop_name"), std::forward_as_tuple(el->name));
            item.emplace(std::piecewise_construct, std::forward_as_tuple("type"), std::forward_as_tuple(std::string("Wait")));
        }
        items_.emplace_back(item);
    }
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("items"), std::forward_as_tuple(items_));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("map"), std::forward_as_tuple(std::move(route_render)));
}

void MapResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("map"), std::forward_as_tuple(std::move(svg_xml_answer)));
}

void CompaniesResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    std::vector<Json::Node> items;
    for (auto & company : companies) {
        for (auto & name : company->names()) {
            if (name.type() == YellowPages::Name_Type_MAIN) {
                items.emplace_back(name.value());
                break;
            }
        }
    }
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("companies"), std::forward_as_tuple(items));
}

void BadResponse::MakeJson() {
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("request_id"), std::forward_as_tuple(id));
    valid_data.emplace(std::piecewise_construct, std::forward_as_tuple("error_message"), std::forward_as_tuple(error_message));
}

void PrintResponses(std::vector<JsonResponse> responses, std::ostream & os) {
    Json::Serializer::Serialize(responses, os);
}