#include "yellow_pages_manager.h"

#include <list>

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
    if (names.empty())
        return;

    auto find_one_of = [&](auto & company) {
        if (std::any_of(names.begin(), names.end(), [&](auto & name) {
            return std::any_of((*company)->names().begin(), (*company)->names().end(), [&](auto & company_name){
                return company_name.value() == name.value();
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
    if (urls.empty())
        return;

    auto find_one_of = [&](auto & company) {
        if (std::any_of(urls.begin(), urls.end(), [&](auto & url) {
            return std::any_of((*company)->urls().begin(), (*company)->urls().end(), [&](auto & url_obj) {
                return url.value() == url_obj.value();
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

void Data_Structure::RubricQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages * db) {
    if (rubrics.empty())
        return;

    auto find_one_of = [&](auto & company) {
        if (std::any_of(rubrics.begin(), rubrics.end(), [&](auto & rubric) {
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
        return std::any_of(object.keywords().begin(), object.keywords().end(), [&](auto & keyword){
            return keyword == query_key;
        });
    });
}

void Data_Structure::PhoneQuery::Compare(std::list<const YellowPages::Company *> & companies, struct DataBaseYellowPages *) {
    if (phones.empty())
        return;

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

    if (query.has_type() && query.type() != object.type()) {
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
