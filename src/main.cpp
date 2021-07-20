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
//        } else if (el["type"].AsString() == "Bus" && el["name"].AsString() == "13" && el["stops"].AsArray().front().AsString() == "Ulitsa Lizy Chaykinoy" && std::next(el["stops"].AsArray().begin())->AsString() == "Pionerskaya ulitsa, 111") {
//            is_ok = true;
//            break;
//        }  else if (el["type"].AsString() == "Bus" && el["name"].AsString() == "297" && el["stops"].AsArray().front().AsString() == "Biryulyovo Zapadnoye") {
//            is_ok = true;
//            break;
//        } else if (el["type"].AsString() == "Bus" && el["name"].AsString() == "289" && el["stops"].AsArray().front().AsString() == "Zagorye") {
//            is_ok = true;
//            break;
//        } else if (el["type"].AsString() == "Stop" && el["name"].AsString() == "Zagorye" && el["longitude"].AsNumber<double>() == 37) {
//            is_ok = true;
//            break;
//        } else if (el["type"].AsString() == "Stop" && el["name"].AsString() == "Vnukovo" && el["road_distances"]["Kiyevskoye sh 10"].AsNumber<int>() == 736717) {
//            is_ok = true;
//            break;
//        } else if (el["type"].AsString() == "Bus" && el["name"].AsString() == "256" && el["stops"].AsArray().front().AsString() == "B") {
//            is_ok = true;
//            break;
//        }
//    }
//
//    if (!is_ok){
//        std::stringstream ss;
//        Json::Serializer::Serialize<std::map<std::string, Json::Node>>(input_map, ss);
//        throw std::logic_error(ss.str());
//    }

    const DS::DataBase db{
            ReadBaseRequests(input_map["base_requests"]),
            ReadRoutingSettings(input_map["routing_settings"]),
            ReadRenderSettings(input_map["render_settings"])
    };

    std::cout << std::setprecision(16);
    PrintResponses(
            ReadStatRequests(db, input_map["stat_requests"]),
            std::cout
    );
    std::cout << std::endl;

    return 0;
}