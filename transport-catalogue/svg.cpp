#include "svg.h"

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit([&out](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        
        if constexpr (std::is_same_v<T, std::string>) {
            out << value;
        } else if constexpr (std::is_same_v<T, Rgb>) {
            out << "rgb("sv << static_cast<int>(value.red) << ","sv
                         << static_cast<int>(value.green) << ","sv
                         << static_cast<int>(value.blue) << ")"sv;
        } else if constexpr (std::is_same_v<T, Rgba>) {
            out << "rgba("sv << static_cast<int>(value.red) << ","sv
                          << static_cast<int>(value.green) << ","sv
                          << static_cast<int>(value.blue) << ","sv
                          << value.opacity << ")"sv;
        }
    }, color);
    
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap cap) {
    switch (cap) {
        case StrokeLineCap::BUTT:       return out << "butt";
        case StrokeLineCap::ROUND:      return out << "round";
        case StrokeLineCap::SQUARE:     return out << "square";
        default:                        return out;
    }
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
    switch (join) {
        case StrokeLineJoin::ARCS:          return out << "arcs";
        case StrokeLineJoin::BEVEL:         return out << "bevel";
        case StrokeLineJoin::MITER:         return out << "miter";
        case StrokeLineJoin::MITER_CLIP:    return out << "miter-clip";
        case StrokeLineJoin::ROUND:         return out << "round";
        default:                            return out;
    }
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << std::endl;
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i) {
        out << points_[i].x << ","sv << points_[i].y;
        if (i != points_.size() - 1) {
            out << " "sv;
        }
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text";


    RenderAttrs(out);
    
    out << " x=\"" << pos_.x << "\"";
    out << " y=\"" << pos_.y << "\"";
    out << " dx=\"" << offset_.x << "\"";
    out << " dy=\"" << offset_.y << "\"";
    out << " font-size=\"" << size_ << "\"";

    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }

    out << ">";

    for (char c : data_) {
        switch (c) {
            case '<':  out << "&lt;"; break;
            case '>':  out << "&gt;"; break;
            case '&':  out << "&amp;"; break;
            case '"':  out << "&quot;"; break;
            case '\'': out << "&apos;"; break;
            default:   out << c; break;
        }
    }

    out << "</text>";
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";

    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) {
        obj->Render(ctx);
    }

    out << "</svg>\n";
}

}  // namespace svg