#include "yellow_pages_manager.h"

#include <list>

#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
using namespace google::protobuf;
using namespace google::protobuf::util;

Data_Structure::DataBaseYellowPages::DataBaseYellowPages(YellowPages::Database new_db) : db(std::move(new_db)) {}

ResponseType Data_Structure::DataBaseYellowPages::FindCompanies(const std::vector<std::shared_ptr<Query>> &querys) {
    std::list<const YellowPages::Company *> companies;
    for (auto & el : db.companies()) {
        companies.push_back(&el);
    }
    for (auto & query : querys){
        query->Compare(companies, this);
    }

    return std::make_shared<CompaniesResponse>(std::vector<const YellowPages::Company *>{companies.begin(), companies.end()});
}

void Data_Structure::NameQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages *) {
    auto find_one_of = [&](auto & company) {
        if (names.empty() || std::any_of(names.begin(), names.end(), [&](auto & name) { return std::any_of((*company)->names().begin(), (*company)->names().end(),
                                                                                          [&](auto & company_name){ return company_name.value() == name.value();
                                                                                                                  });
                                                                              }))
            return true;
        return false;
    };
    for (auto company_it = companies.begin(); company_it != companies.end();) {
        if (!find_one_of(company_it))
            company_it = companies.erase(company_it);
        else
            company_it++;
    }
}

void Data_Structure::UrlQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages *) {
    auto find_one_of = [&](auto & company) {
        if (urls.empty() || std::any_of(urls.begin(), urls.end(), [&](auto & url) {
            for (auto & url_obj : (*company)->urls()) {
                if (url_obj.value() == url.value())
                    return true;
            }
            return false;
        }))
            return true;
        return false;
    };
    for (auto company_it = companies.begin(); company_it != companies.end();) {
        if (!find_one_of(company_it))
            company_it = companies.erase(company_it);
        else
            company_it++;
    }
}

void Data_Structure::RubricQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages * db) {
    auto find_one_of = [&](auto & company) {
        if (rubrics.empty() || std::any_of(rubrics.begin(), rubrics.end(), [&](auto & rubric) {
           return std::any_of((*company)->rubrics().begin(), (*company)->rubrics().end(), [&](auto rubric_id){
                return DoesRubricMatch(rubric, db->GetRubric(rubric_id));
            });
        }))
            return true;
        return false;
    };
    for (auto company_it = companies.begin(); company_it != companies.end();) {
        if (!find_one_of(company_it))
            company_it = companies.erase(company_it);
        else
            company_it++;
    }
}

bool Data_Structure::RubricQuery::DoesRubricMatch(const YellowPages::Rubric &query, const YellowPages::Rubric &object) {
    return std::any_of(query.keywords().begin(), query.keywords().end(), [&](auto & query_key) {
          for (auto & keyword : object.keywords()){
              if (keyword == query_key)
                  return true;
          }
          return false;
    });
}

void Data_Structure::PhoneQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages *) {
    auto find_one_of = [&](auto & company) {
        if (phones.empty() || std::any_of(phones.begin(), phones.end(), [&](auto & phone) {
            return std::any_of((*company)->phones().begin(), (*company)->phones().end(), [&](auto & object) {
                return DoesPhoneMatch(phone, object);
            });
        }))
            return true;
        return false;
    };
    for (auto company_it = companies.begin(); company_it != companies.end();) {
        if (!find_one_of(company_it))
            company_it = companies.erase(company_it);
        else
            company_it++;
    }
}

bool
Data_Structure::PhoneQuery::DoesPhoneMatch(const YellowPages::Phone &query, const YellowPages::Phone &object)  {
    if (!query.extension().empty() && query.extension() != object.extension()) {
        return false;
    }

    auto reflection = query.GetReflection();
    auto description = query.GetDescriptor();
    auto fd = description->field(1);
    if (reflection->HasField(query, fd) && query.type() != object.type()) {
        return false;
    }
    if (!query.country_code().empty() && query.country_code() != object.country_code()) {
        return false;
    }
    if (
            (!query.local_code().empty() || !query.country_code().empty())
            && query.local_code() != object.local_code()
            ) {
        return false;
    }
    return query.number() == object.number();
}
