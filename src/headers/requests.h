#ifndef REQUESTS_H
#define REQUESTS_H

#include <iostream>

#include "json.h"
#include "data_manager.h"
#include <functional>

namespace DS = Data_Structure;
using RequestType = std::unique_ptr<struct IRequest>;

struct IRequest {
public:
    enum class Type {
        READ_STOP,
        READ_BUS,
        FIND_STOP,
        FIND_BUS,
        FIND_ROUTE,
        BUILD_MAP
    };
    virtual void ParseFrom(Json::Node const & json_node) = 0;
    virtual ~IRequest() = default;
};

struct ExecuteRequest : public  IRequest {
public:
    inline static const std::map<std::string_view, Type> AsType{
            {"bus", Type::FIND_BUS},
            {"stop", Type::FIND_STOP},
            {"route", Type::FIND_ROUTE},
            {"map", Type::BUILD_MAP}
    };
    inline static int requests_count = 0;
    void ParseFrom(Json::Node const & json_node) override {
        id = json_node["id"].AsNumber<int>();
    }
    virtual JsonResponse Process(const DS::DataBase &) = 0;
    virtual ~ExecuteRequest() = default;

    template <typename F, typename ... Args>
    JsonResponse ProcessResponse(F&& func, Args&& ...args){
        auto resp = std::bind(std::forward<F>(func), args...)();
        resp->id = id;
        resp->MakeJson();
        return resp->GetJson();
    }
protected:
    int id{};
};

struct FindBusRequest final : public ExecuteRequest {
public:
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        ExecuteRequest::ParseFrom(json_node);
        title = json_node["name"].AsString();
    }
private:
    std::string title;
};

struct FindStopRequest final : public ExecuteRequest {
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        ExecuteRequest::ParseFrom(json_node);
        title = json_node["name"].AsString();
    }
private:
    std::string title;
};

struct FindRouteRequest final : public ExecuteRequest {
public:
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        ExecuteRequest::ParseFrom(json_node);
        from = json_node["from"].AsString();
        to = json_node["to"].AsString();
    }
private:
    std::string from, to;
};

struct MapRouteRequest final : public ExecuteRequest{
public:
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        ExecuteRequest::ParseFrom(json_node);
    }
};

struct ModifyRequest : public IRequest {
public:
    using IRequest::IRequest;
    inline static const std::map<std::string_view, Type> AsType{
            {"bus", Type::READ_BUS},
            {"stop", Type::READ_STOP}
    };
    void ParseFrom(Json::Node const & json_node) override {
        title = json_node["name"].AsString();
    }
    virtual DS::DBItem Process() = 0;
    virtual ~ModifyRequest() = default;
protected:
    std::string title;
};

struct AddStopRequest final : public ModifyRequest {
public:
    DS::DBItem Process() override;
    void ParseFrom(Json::Node const & json_node) override {
        ModifyRequest::ParseFrom(json_node);
        dist = Distance{
            json_node["longitude"].AsNumber<double>(),
            json_node["latitude"].AsNumber<double>()
        };
        for (auto & [key, value] : json_node["road_distances"].AsMap()){
            adjacent_stops.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(value.AsNumber<int>())
                    );
        }
    }
private:
    Distance dist {};
    std::map<std::string, int> adjacent_stops;
};

struct AddBusRequest final : public ModifyRequest {
public:
    DS::DBItem Process() override;
    void ParseFrom(Json::Node const & json_node) override {
        ModifyRequest::ParseFrom(json_node);
        stops.reserve(json_node["stops"].AsArray().size());
        for (auto & el : json_node["stops"].AsArray()){
            stops.push_back(el.AsString());
        }
        is_roundtrip = json_node["is_roundtrip"].AsBool();
        if (!is_roundtrip){
            bool is_first = true;
            for (auto & el : Ranges::Reverse(Ranges::AsRange(json_node["stops"].AsArray()))){
                if (!is_first)
                    stops.push_back(el.AsString());
                is_first = false;
            }
        }
    }
private:
    std::vector<std::string> stops;
    bool is_roundtrip {};
};

RequestType CreateRequest(IRequest::Type type);

std::vector<DS::DBItem> ReadBaseRequests(Json::Node const & input);
std::pair<double, int> ReadRoutingSettings(Json::Node const & input);
std::vector<JsonResponse> ReadStatRequests(const DS::DataBase & db, Json::Node const & input);

#endif //REQUESTS_H
