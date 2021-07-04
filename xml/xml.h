#ifndef XML_H
#define XML_H

#include <iostream>
#include <utility>
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace XML {
    struct config{
        std::string open;
        std::string close;
        std::vector<std::pair<std::string, struct xml>> data;
    };

    struct config_with_parameters : public config{
        using config::config;
        config_with_parameters();
        std::shared_ptr<struct xml> parameters;
    };

    struct text {
        text() = default;
        explicit text(std::string str) : raw_str(std::move(str)) {}
        std::string raw_str;
    };

    using variant_inheritance = std::variant<std::string, config, config_with_parameters, text, std::vector<struct xml>>;
    using Array = std::vector<std::pair<std::string, struct xml>>;

    struct xml : variant_inheritance {
    public:
        using variant_inheritance::variant_inheritance;

        [[nodiscard]] inline const variant_inheritance &GetOrigin() const {
            return *this;
        }

        [[nodiscard]] const std::string &AsString() const {
            return std::get<std::string>(*this);
        }

        [[nodiscard]] const text & AsText() const {
            return std::get<text>(*this);
        }

        [[nodiscard]] const config & AsConfig() const {
            return std::get<config>(*this);
        }

        [[nodiscard]] const config & AsConfigWithParameters() const {
            return std::get<config_with_parameters>(*this);
        }

        [[nodiscard]] const std::vector<std::pair<std::string, struct xml>> &AsArray() const {
            return std::get<config>(*this).data;
        }
    };

    namespace Serializer {
        template <typename T>
        void Serialize(const T & xml_node, std::ostream &out = std::cout);

        template <>
        void Serialize<std::string>(const std::string & xml_str, std::ostream &out);

        template <>
        void Serialize<text>(const text & xml_str, std::ostream &out);

        template <>
        void Serialize<config>(const config & xml_node, std::ostream &out);

        template <>
        void Serialize<config_with_parameters>(const config_with_parameters & xml_node, std::ostream &out);

        template <>
        void Serialize<std::vector<XML::xml>>(const std::vector<XML::xml> & xml_vect, std::ostream &out);

        template <>
        void Serialize<xml>(const xml & xml_node, std::ostream & out);
    }
}


#endif //XML_H
