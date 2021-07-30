#include <iostream>

#include "requests.h"
#include "responses.h"
#include "data_manager.h"

int main(){
    const auto doc = Json::Load(std::cin);
    const auto &input_map = doc.GetRoot();

//    bool is_ok = false;
//    auto base_reqs = input_map["base_requests"];
//    for (auto & el : base_reqs.AsArray()) {
//        if (el["type"].AsString() == "Bus" && el["name"].AsString() == "23" && el["stops"].AsArray().front().AsString() == "Sanatoriy Salyut"){
//            is_ok = true;
//            break;
//        }
//        else if (el["type"].AsString() == "Bus" && el["name"].AsString() == "14" && el["stops"].AsArray().front().AsString() == "Ulitsa Lizy Chaykinoy") {
//            is_ok = true;
//            break;
//        }
//    }
//
//    if (!is_ok){
//        std::stringstream ss;
//        Json::Serializer::Serialize(input_map.AsMap(), ss);
//        throw std::logic_error(ss.str());
//    }

    const DS::DataBase db{
            ReadBaseRequests(input_map["base_requests"]),
            ReadRoutingSettings(input_map["routing_settings"]),
            ReadRenderSettings(input_map["render_settings"])
    };

    PrintResponses(
            ReadStatRequests(db, input_map["stat_requests"]),
            std::cout
    );
    std::cout << std::endl;

    return 0;
}