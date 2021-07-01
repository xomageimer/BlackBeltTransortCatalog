#include "json.h"

Json::Node Json::Deserializer::LoadNode(std::istream &input) {
    char c;
    input >> c;
    if (c == '{'){
        return Json::Deserializer::LoadMap(input);
    } else if (c == '[') {
        return Json::Deserializer::LoadArray(input);
    } else if (c == '"') {
        return Json::Deserializer::LoadString(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return Json::Deserializer::LoadBool(input);
    } else {
        input.putback(c);
        return Json::Deserializer::LoadNumber(input);
    }
}

Json::Node Json::Deserializer::LoadMap(std::istream &input) {
    std::map<std::string, Node> dict;

    for (char c {}; input >> c && c != '}';){
        if (c == ',')
            input >> c;

        std::string key = LoadString(input).AsString();
        input >> c;
        dict.emplace(key, LoadNode(input));
    }
    return Node{std::move(dict)};
}

Json::Node Json::Deserializer::LoadArray(std::istream &input) {
    std::vector<Node> arr;

    for (char c{}; input >> c && c != ']';){
        if (c != ',')
            input.putback(c);

        arr.emplace_back(LoadNode(input));
    }
    return Node{std::move(arr)};
}

Json::Node Json::Deserializer::LoadString(std::istream &input) {
    std::string str;
    std::getline(input, str, '"');

    return Node{std::move(str)};
}

Json::Node Json::Deserializer::LoadBool(std::istream &input) {
    std::string flag;
    char c{};
    for (; input >> c && c != 'e';)
        flag.push_back(c);
    flag.push_back(c);

    return Node{std::string("true") == flag};
}

Json::Node Json::Deserializer::LoadNumber(std::istream &input) {
    int value = 0;
    double sign = 1;
    if (input.peek() == '-'){
        input.get();
        sign = -1;
    }
    input >> value;
    if (input.peek() == '.'){
        input.get();
        double new_value = value;
        double div = 10;
        while (isdigit(input.peek())){
            new_value += (input.get() - '0') / div;
            div *= 10;
        }
        return {sign * new_value};
    }
    return {static_cast<int>(sign * value)};
}

namespace Json::Serializer{
    template<>
    void Serialize<bool>(const Json::Node &node, std::ostream &output) {
        output << (node.AsBool() ? "true" : "false");
    }

    template<>
    void Serialize<std::string>(const Json::Node &node, std::ostream &output) {
        output << std::quoted(node.AsString());
    }

    template<>
    void Serialize<std::vector<Node>>(const Json::Node &node, std::ostream &output) {
        output << "[";
        bool is_first = true;
        for (const auto &el : node.AsArray()) {
            if (!is_first)
                output << ", ";
            is_first = false;
            std::visit([&output](auto const &arg) {
                Serialize<std::decay_t<decltype(arg)>>(arg, output);
            }, el.GetOrigin());
        }
        output << "]";
    }

    template<>
    void Serialize<std::map<std::string, Node>>(const Json::Node &node, std::ostream &output) {
        output << "{";
        bool is_first = true;
        for (const auto &[key, value] : node.AsMap()) {
            if (!is_first)
                output << ", ";
            is_first = false;
            output << std::quoted(key) << ": ";
            std::visit([&output](auto const &arg) {
                Serialize<std::decay_t<decltype(arg)>>(arg, output);
            }, value.GetOrigin());
        }
        output << "}";
    }
}