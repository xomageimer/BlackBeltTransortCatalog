#include "xml.h"

#include <iomanip>

template<>
void XML::Serializer::Serialize<std::string>(const std::string &xml_str, std::ostream &out) {
    out << std::quoted(xml_str);
}

template<>
void XML::Serializer::Serialize<XML::text>(const XML::text &xml_str, std::ostream &out) {
    out << xml_str.raw_str;
}


template<>
void XML::Serializer::Serialize<XML::config>(const XML::config &xml_node, std::ostream &out) {
    out << "<" << xml_node.open << " ";
    for (auto & [key, value] : xml_node.data){
        out << key << "=";
        std::visit([&out](const auto & arg){
            Serialize(arg, out);
        }, value.GetOrigin());
        out << " ";
    }
    out << xml_node.close << ">";
}

template<>
void XML::Serializer::Serialize<XML::config_with_parameters>(const XML::config_with_parameters &xml_node, std::ostream &out) {
    out << "<" << xml_node.open << " ";
    for (auto & [key, value] : xml_node.data){
        out << key << "=";
        std::visit([&out](const auto & arg){
            Serialize(arg, out);
        }, value.GetOrigin());
        out << " ";
    }
    out << ">";
    std::visit([&out](const auto & arg){
        Serialize(arg, out);
    }, xml_node.parameters->GetOrigin());
    out << "<" << xml_node.close << ">";
}

template<>
void XML::Serializer::Serialize<XML::xml>(const XML::xml &xml_node, std::ostream &out) {
    std::visit([&out](const auto & arg){
        Serialize(arg, out);
    }, xml_node.GetOrigin());
}

template<>
void XML::Serializer::Serialize<std::vector<XML::xml>>(const std::vector<XML::xml> &xml_vect, std::ostream &out) {
    for (auto & xml_node : xml_vect){
        std::visit([&out](const auto & arg){
            Serialize<std::decay_t<decltype(arg)>>(arg, out);
        }, xml_node.GetOrigin());
    }
}

XML::config_with_parameters::config_with_parameters() {
    parameters = std::make_shared<xml>();
}