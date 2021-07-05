#include "requests.h"

std::vector<DS::DBItem> ReadBaseRequests(const Json::Node &input) {
    std::vector<DS::DBItem> elements;
    RequestType req_handler;
    for (auto const & el : input.AsArray()){
        req_handler = CreateRequest(ModifyRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        elements.emplace_back(reinterpret_cast<ModifyRequest *>(req_handler.get())->Process());
    }
    return std::move(elements);
}

std::pair<double, int> ReadRoutingSettings(const Json::Node &input) {
    return {static_cast<double>(input["bus_wait_time"].AsNumber<int>()), input["bus_velocity"].AsNumber<int>()};
}

std::vector<JsonResponse> ReadStatRequests(const DS::DataBase &db, const Json::Node &input) {
    std::vector<JsonResponse> responses;
    RequestType req_handler;
    for (auto const & el : input.AsArray()){
        req_handler = CreateRequest(ExecuteRequest::AsType.at(el["type"].AsString()));
        req_handler->ParseFrom(el);
        responses.emplace_back(reinterpret_cast<ExecuteRequest *>(req_handler.get())->Process(db));
    }

    return std::move(responses);
}

DS::DBItem AddStopRequest::Process() {
   return DS::Stop{std::move(title), dist, std::move(adjacent_stops)};
}

DS::DBItem AddBusRequest::Process() {
    return DS::Bus{std::move(title), std::move(stops), is_roundtrip};
}

JsonResponse FindBusRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindBus, std::ref(db), std::ref(title));
}

JsonResponse FindStopRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindStop, std::ref(db), std::ref(title));
}

JsonResponse FindRouteRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::FindRoute, std::ref(db), std::ref(from), std::ref(to));
}

JsonResponse MapRouteRequest::Process(const DS::DataBase & db) {
    return ProcessResponse(&DS::DataBase::BuildMap, std::ref(db));
}

RequestType CreateRequest(IRequest::Type type) {
    switch(type){
        case IRequest::Type::READ_BUS:
            return std::make_unique<AddBusRequest>();
        case IRequest::Type::READ_STOP:
            return std::make_unique<AddStopRequest>();
        case IRequest::Type::FIND_BUS:
            return std::make_unique<FindBusRequest>();
        case IRequest::Type::FIND_STOP:
            return std::make_unique<FindStopRequest>();
        case IRequest::Type::FIND_ROUTE:
            return std::make_unique<FindRouteRequest>();
        case IRequest::Type::BUILD_MAP:
            return std::make_unique<MapRouteRequest>();
    }
    throw std::logic_error("Bad type");
}