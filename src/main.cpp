#include <iostream>

#include "requests.h"
#include "responses.h"
#include "data_manager.h"

int main(){
    const auto doc = Json::Load(std::cin);
    const auto &input_map = doc.GetRoot();

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