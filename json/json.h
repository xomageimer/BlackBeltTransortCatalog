#ifndef JSON_H
#define JSON_H

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <iomanip>

#include "xml.h"

namespace Json {
    template<typename T>
    struct is_number : std::false_type {};

    template<>
    struct is_number<int> : std::true_type {};
    template<>
    struct is_number<double> : std::true_type {};

    struct Node;
    using variant_inheritance = std::variant<int, double, bool, std::string, std::vector<Node>, std::map<std::string, Node>, XML::xml>;

    struct Node : public Json::variant_inheritance {
    public:
        using variant_inheritance::variant_inheritance;

        [[nodiscard]] inline const Json::variant_inheritance &GetOrigin() const {
            return *this;
        }

        template<typename T>
        [[nodiscard]] inline std::enable_if_t<!is_number<std::decay_t<T>>::value, T> AsNumber() const {
            std::cout << "T is not a \"is_number\" type (int, double)" << std::endl;
        }

        template<typename T>
        [[nodiscard]] inline std::enable_if_t<is_number<std::decay_t<T>>::value, T> AsNumber() const {
            return std::get<T>(*this);
        }

        [[nodiscard]] inline bool const &AsBool() const {
            return std::get<bool>(*this);
        }

        [[nodiscard]] inline std::string const &AsString() const {
            return std::get<std::string>(*this);
        }

        [[nodiscard]] inline std::vector<Node> const &AsArray() const {
            return std::get<std::vector<Node>>(*this);
        }

        [[nodiscard]] inline std::map<std::string, Node> const &AsMap() const {
            return std::get<std::map<std::string, Node>>(*this);
        }

        [[nodiscard]] inline XML::xml const &AsXML() const {
            return std::get<XML::xml>(*this);
        }

        [[nodiscard]] const Json::Node & operator[](std::string const & key) const {
            return AsMap().at(key);
        }

        [[nodiscard]] const Json::Node & operator[](size_t i) const {
            return AsArray().at(i);
        }
    };
    template<>
    [[nodiscard]] inline double Node::AsNumber<double>() const {
        return (std::holds_alternative<double>(*this) ? std::get<double>(*this) : std::get<int>(*this));
    }

    struct Document {
    private:
        Node root;
    public:
        explicit Document(Node new_root) : root(std::move(new_root)) {};

        [[nodiscard]] inline Node const &GetRoot() const {
            return root;
        }
    };

    namespace Deserializer {
        Node LoadNode(std::istream &input);

        Node LoadMap(std::istream &input);

        Node LoadArray(std::istream &input);

        Node LoadString(std::istream &input);

        Node LoadNumber(std::istream &input);

        Node LoadBool(std::istream &input);
    }

    inline Document Load(std::istream &input) {
        return std::move(Document{Deserializer::LoadNode(input)});
    }

    // TODO переделать сериалайзер чтобы не нужно было задавать шаблон явно!
    namespace Serializer {
        template<typename T>
        std::enable_if_t<!is_number<T>::value, void> Serialize(const Json::Node &node, std::ostream &output = std::cout);

        template<>
        void Serialize<bool>(const Json::Node &node, std::ostream &output);

        template<typename T>
        std::enable_if_t<is_number<T>::value, void> Serialize(const Json::Node &node, std::ostream &output) {
            output << node.AsNumber<T>();
        }

        template<>
        void Serialize<XML::xml>(const Json::Node &node, std::ostream &output);

        template<>
        void Serialize<std::string>(const Json::Node &node, std::ostream &output);

        template<>
        void Serialize<std::vector<Node>>(const Json::Node &node, std::ostream &output);

        template<>
        void Serialize<std::map<std::string, Node>>(const Json::Node &node, std::ostream &output);
    }
}

#endif //JSON_H