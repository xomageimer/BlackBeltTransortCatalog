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
        BUILD_MAP,
        FIND_COMPANIES,
        FIND_ROUTE_COMPANY
    };
    virtual void ParseFrom(Json::Node const & json_node) = 0;
    virtual ~IRequest() = default;
};

struct ExecuteRequest : public  IRequest {
public:
    inline static const std::map<std::string_view, Type> AsType{
            {"Bus", Type::FIND_BUS},
            {"Stop", Type::FIND_STOP},
            {"Route", Type::FIND_ROUTE},
            {"Map", Type::BUILD_MAP},
            {"FindCompanies", Type::FIND_COMPANIES},
            {"RouteToCompany", Type::FIND_ROUTE_COMPANY}
    };
    inline static int requests_count = 0;
    void ParseFrom(Json::Node const & json_node) override {
        id = json_node["id"].AsNumber<int>();
    }
    virtual JsonResponse Process(const DS::DataBase &) = 0;
    virtual ~ExecuteRequest() = default;

    template <typename F, typename ... Args>
    JsonResponse ProcessResponse(F&& func, Args&& ...args){
        auto resp = std::bind(std::forward<F>(func), std::forward<Args>(args)...)();
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


YellowPages::Rubric ParseRubric(Json::Node const & json_node);
YellowPages::Name ParseName(Json::Node const & json_node);
YellowPages::Phone ParsePhone(Json::Node const & json_node);
YellowPages::Url ParseUrl(Json::Node const & json_node);
#define ParseLike(struct_name, type_name)                                                        \
{                                                                                            \
    std::vector<YellowPages::struct_name> objects;                                           \
    if (json_node.AsMap().count(#type_name)) {                                               \
        for (auto & object : json_node[#type_name].AsArray()) {                              \
            objects.emplace_back(Parse ## struct_name(object));                              \
        }                                                                                    \
    }                                                                                        \
    if (!objects.empty())                                                                    \
        querys.emplace_back(std::make_shared<DS::struct_name ## Query>(std::move(objects))); \
}
struct FindCompaniesRequest : public ExecuteRequest {
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        ExecuteRequest::ParseFrom(json_node);
        ParseQueries(json_node);
    }
    void ParseQueries(Json::Node const & json_node){
        ParseLike(Rubric, rubrics);
        ParseLike(Phone, phones);
        ParseLike(Url, urls);
        ParseLike(Name, names);
    }
protected:
    std::vector<std::shared_ptr<DS::Query>> querys;
};

struct FindRouteToCompaniesRequest final : public FindCompaniesRequest {
    JsonResponse Process(const DS::DataBase &) override;
    void ParseFrom(Json::Node const & json_node) override {
        FindCompaniesRequest::ExecuteRequest::ParseFrom(json_node);
        FindCompaniesRequest::ParseQueries(json_node["companies"]);

        from = json_node["from"].AsString();
    }
private:
    std::string from;
};
#undef ParseLike

struct ModifyRequest : public IRequest {
public:
    using IRequest::IRequest;
    inline static const std::map<std::string_view, Type> AsType{
            {"Bus", Type::READ_BUS},
            {"Stop", Type::READ_STOP}
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
DS::RoutingSettings ReadRoutingSettings(Json::Node const & input);
std::vector<JsonResponse> ReadStatRequests(const DS::DataBase & db, Json::Node const & input);
DS::RenderSettings ReadRenderSettings(Json::Node const & input);
YellowPages::Database ReadYellowPagesData(Json::Node const & input);

#endif //REQUESTS_H
