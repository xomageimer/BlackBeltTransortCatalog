#ifndef SVG_H
#define SVG_H

#include "xml.h"

#include <iostream>

#include <variant>
#include <string>
#include <sstream>

// TODO убрать to_string, все примитивы должны выводится в ostream
namespace Svg {
    struct Rgba{
        uint8_t red, green, blue;
        std::optional<double> alpha;
    };
    struct Color {
        Color() : color(std::monostate{}) {};
        Color(std::string const & color_str) : color(color_str) {};
        Color(const char * color_str) : color(color_str) {};
        Color(Svg::Rgba const & color_rgb) : color(color_rgb) {}

        inline bool IsNone() const {
            return std::holds_alternative<std::monostate>(color);
        }

        explicit operator std::string() const{
            if (std::holds_alternative<Rgba>(color)) {
                auto const & rgb_c = std::get<Rgba>(color);
                if (!rgb_c.alpha)
                    return std::string("rgb(" + std::to_string(rgb_c.red) + ", "
                        + std::to_string(rgb_c.green) + ", "
                            + std::to_string(rgb_c.blue)) + ")";
                else {
                    std::stringstream ss_prec;
                    ss_prec << *rgb_c.alpha;
                    return std::string("rgba(" + std::to_string(rgb_c.red) + ", "
                                       + std::to_string(rgb_c.green) + ", "
                                       + std::to_string(rgb_c.blue)) + ", " + ss_prec.str() + ")";
                }
            } else {
                return std::string(std::get<std::string>(color));
            }
        }

    private:
        std::variant<std::monostate, std::string, Rgba> color;
    };
    [[maybe_unused]] const Color NoneColor {};

    struct Point{
        double x{0.}, y{0.};
    };
    inline bool operator<(Point const & lhs, Point const & rhs){
        return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
    }

    template <typename cur_prim>
    struct Primitive {
    public:
        cur_prim & SetStrokeColor(const Color & color) & {
            stroke_color = color;
            return *static_cast<cur_prim *>(this);
        }
        cur_prim & SetFillColor(const Color & color) & {
            fill_color = color;
            return *static_cast<cur_prim *>(this);
        }
        cur_prim & SetStrokeWidth(double width) & {
            stroke_width = width;
            return *static_cast<cur_prim *>(this);
        }
        cur_prim & SetStrokeLineCap(const std::string& linecap) & {
            type_of_stroke_linecap = linecap;
            return *static_cast<cur_prim *>(this);
        }
        cur_prim & SetStrokeLineJoin(const std::string& linejoin) & {
            type_of_stroke_linejoin = linejoin;
            return *static_cast<cur_prim *>(this);
        }

        cur_prim && SetStrokeColor(const Color & color) && {
            stroke_color = color;
            return std::move(*static_cast<cur_prim *>(this));
        }
        cur_prim && SetFillColor(const Color & color) && {
            fill_color = color;
            return std::move(*static_cast<cur_prim *>(this));
        }
        cur_prim && SetStrokeWidth(double width) && {
            stroke_width = width;
            return std::move(*static_cast<cur_prim *>(this));
        }
        cur_prim && SetStrokeLineCap(const std::string& linecap) && {
            type_of_stroke_linecap = linecap;
            return std::move(*static_cast<cur_prim *>(this));
        }
        cur_prim && SetStrokeLineJoin(const std::string& linejoin) && {
            type_of_stroke_linejoin = linejoin;
            return std::move(*static_cast<cur_prim *>(this));
        }

        [[nodiscard]] XML::config MakeXml() const;
    protected:
        Primitive() = default;

        Color stroke_color {NoneColor};
        Color fill_color {NoneColor};

        double stroke_width {1.0};

        std::string type_of_stroke_linecap;
        std::string type_of_stroke_linejoin;
    };

    struct Polyline : public Primitive<Polyline>{
    public:
        Polyline & AddPoint(Point) & ;

        Polyline && AddPoint(Point) && ;

        [[nodiscard]] XML::xml MakeXml() const;
    private:
        std::vector<Point> points;
    };

    struct Circle : public Primitive<Circle> {
    public:
        Circle & SetCenter(Point) &;
        Circle & SetRadius(double) &;

        Circle && SetCenter(Point) &&;
        Circle && SetRadius(double) &&;

        [[nodiscard]] XML::xml MakeXml() const;
    private:
        Point point;
        double radius {1.0};
    };

    struct Text : public Primitive<Text> {
    public:
        Text & SetPoint(Point) &;
        Text & SetOffset(Point) &;
        Text & SetFontSize(uint32_t) &;
        Text & SetFontFamily(const std::string &) &;
        Text & SetData(const std::string &) &;
        Text & SetFontWeight(const std::string &) &;

        Text && SetPoint(Point) &&;
        Text && SetOffset(Point) &&;
        Text && SetFontSize(uint32_t) &&;
        Text && SetFontFamily(const std::string &) &&;
        Text && SetData(const std::string &) &&;
        Text && SetFontWeight(const std::string &) &&;

        [[nodiscard]] XML::xml MakeXml() const;
    private:
        Point point;
        Point offset;
        uint32_t font_size {1};
        std::string font_family;
        std::string font_weight;
        XML::text text;
    };

    template <typename C, typename P>
    struct is_inherited{
    private:
        struct TRUE_{bool m;};
        struct FALSE_{bool m[2];};
        static TRUE_ check(P *) { return TRUE_{};}
        static FALSE_ check(...) {return FALSE_{};}
    public:
        static bool const value = sizeof(check((C*)(nullptr))) == sizeof(TRUE_);
    };

    using sigma_types = std::variant<Polyline, Circle, Text>;

    struct Document {
    public:
        Document();
        template <typename T>
        std::enable_if_t<is_inherited<std::decay_t<T>, Primitive<std::decay_t<T>>>::value, void> Add(T && obj) noexcept {
            primitives.emplace_back(std::forward<T>(obj));
        }
        template <typename T>
        std::enable_if_t<!(is_inherited<T, Primitive<T>>::value), void> Add(T) noexcept {
            throw std::logic_error("Trying to add wrong type object!");
        }

        void Render(std::ostream & out = std::cout);
        void SimpleRender();

        [[nodiscard]] XML::xml Get() const;
    private:
        std::vector<sigma_types> primitives;
        std::vector<XML::xml> xml_s;
    };
}


#endif //SVG_H
