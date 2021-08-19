#ifndef YELLOW_PAGES_MANAGER_H
#define YELLOW_PAGES_MANAGER_H

#include <string>
#include <list>
#include <vector>

#include "responses.h"

#include "database.pb.h"
#include "company.pb.h"
#include "rubric.pb.h"

namespace Data_Structure {
    struct Query {
    public:
        virtual void Compare(std::list<const YellowPages::Company *> &, struct DataBaseYellowPages * = nullptr) = 0;
        virtual ~Query() = default;
    };
    struct NameQuery : public Query {
    public:
        explicit NameQuery(std::vector<YellowPages::Name> quer_names) : names(std::move(quer_names)) {}
        void Compare(std::list<const YellowPages::Company *> &, struct DataBaseYellowPages *)  override;
    private:
        std::vector<YellowPages::Name> names;
    };
    struct UrlQuery : public Query {
    public:
        explicit UrlQuery(std::vector<YellowPages::Url> guer_urls) : urls(std::move(guer_urls)) {}
        void Compare(std::list<const YellowPages::Company *> &, struct DataBaseYellowPages *)  override;
    private:
        std::vector<YellowPages::Url> urls;
    };
    struct RubricQuery : public Query {
    public:
        explicit RubricQuery(std::vector<YellowPages::Rubric> quer_rubrics) : rubrics(std::move(quer_rubrics)) {}
        void Compare(std::list<const YellowPages::Company *> &, struct DataBaseYellowPages *)  override;
    private:
        static bool DoesRubricMatch(const YellowPages::Rubric& query, const YellowPages::Rubric& object);
        std::vector<YellowPages::Rubric> rubrics;
    };
    struct PhoneQuery : Query {
    public:
        explicit PhoneQuery(std::vector<YellowPages::Phone> quer_phones) : phones(std::move(quer_phones)) {}
        void Compare(std::list<const YellowPages::Company *> &, struct DataBaseYellowPages *) override;
    private:
        static bool DoesPhoneMatch(const YellowPages::Phone& query, const YellowPages::Phone& object);
        std::vector<YellowPages::Phone> phones;
    };

    struct DataBaseYellowPages {
    public:
        explicit DataBaseYellowPages(YellowPages::Database new_db);
        inline YellowPages::Database Serialize() {
            return db;
        };
        const YellowPages::Database & GetOrigin() const {
            return db;
        };
        [[nodiscard]] YellowPages::Rubric const & GetRubric(size_t id) const{
            return db.rubrics().at(id);
        }
        [[nodiscard]] ResponseType FindCompanies(const std::vector<std::shared_ptr<Query>> & querys);
    private:
        YellowPages::Database db;
    };
}
#endif //YELLOW_PAGES_MANAGER_H
