#include "json.h"
#include <cctype>
#include <cassert>
#include <sstream>
#include <type_traits> 

namespace json {

namespace {

void PrintNode(const Node& node, const struct PrintContext& ctx);

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) out.put(' ');
    }

    PrintContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }
};

std::string LoadString(std::istream& input) {
    std::string s;
    char ch;
    bool found_closing_quote = false;
    while (input.get(ch)) {
        if (ch == '"') {
            found_closing_quote = true;
            break;
        }
        if (ch == '\\') {
            if (!input.get(ch)) throw ParsingError("String parsing error");
            switch (ch) {
                case 'n': s.push_back('\n'); break;
                case 't': s.push_back('\t'); break;
                case 'r': s.push_back('\r'); break;
                case '"': s.push_back('"'); break;
                case '\\': s.push_back('\\'); break;
                default: throw ParsingError(std::string("Unrecognized escape sequence \\") + ch);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line");
        } else {
            s.push_back(ch);
        }
    }
    if (!found_closing_quote) {
        throw ParsingError("String parsing error");
    }
    return s;
}


std::variant<int, double> LoadNumber(std::istream& input) {
    std::string num_str;
    auto read_char = [&]() {
        char c = static_cast<char>(input.get());
        if (!input) throw ParsingError("Failed to read number");
        num_str += c;
    };

    if (input.peek() == '-') read_char();

    if (input.peek() == '0') {
        read_char();
    } else if (std::isdigit(static_cast<unsigned char>(input.peek()))) {
        while (std::isdigit(static_cast<unsigned char>(input.peek()))) read_char();
    } else {
        throw ParsingError("Invalid number format");
    }

    bool is_int = true;

    if (input.peek() == '.') {
        is_int = false;
        read_char();
        if (!std::isdigit(static_cast<unsigned char>(input.peek()))) throw ParsingError("Invalid fractional part");
        while (std::isdigit(static_cast<unsigned char>(input.peek()))) read_char();
    }

    if (input.peek() == 'e' || input.peek() == 'E') {
        is_int = false;
        read_char();
        if (input.peek() == '+' || input.peek() == '-') read_char();
        if (!std::isdigit(static_cast<unsigned char>(input.peek()))) throw ParsingError("Invalid exponent");
        while (std::isdigit(static_cast<unsigned char>(input.peek()))) read_char();
    }

    try {
        if (is_int) {
            try {
                return std::stoi(num_str);
            } catch (...) {}
        }
        return std::stod(num_str);
    } catch (...) {
        throw ParsingError(std::string("Failed to convert number: ") + num_str);
    }
}

Node LoadNode(std::istream& input);

Array LoadArray(std::istream& input) {
    Array arr;
    char c;
    while (input >> c) {
        if (c == ']') return arr;
        if (c != ',') input.putback(c);
        arr.push_back(LoadNode(input));
    }
    throw ParsingError("Array parsing error");
}

Dict LoadDict(std::istream& input) {
    Dict dict;
    char c;
    while (input >> c) {
        if (c == '}') return dict;
        if (c == ',') input >> c;
        if (c != '"') throw ParsingError("Key must be a string");
        std::string key = LoadString(input);
        if (!(input >> c) || c != ':') throw ParsingError("Colon expected");
        dict.emplace(std::move(key), LoadNode(input));
    }
    throw ParsingError("Dict parsing error");
}

Node LoadNode(std::istream& input) {
    char c;
    while (input.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;

        if (c == '[') return Node{LoadArray(input)};
        if (c == '{') return Node{LoadDict(input)};
        if (c == '"') return Node{LoadString(input)};

        // Числа
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            input.putback(c);
            auto num = LoadNumber(input);
            if (std::holds_alternative<int>(num)) {
                return Node{std::get<int>(num)};
            } else {
                return Node{std::get<double>(num)};
            }
        }

        // Буквы
        if (std::isalpha(static_cast<unsigned char>(c))) {
            std::string token;
            token += c; 
            while (std::isalpha(static_cast<unsigned char>(input.peek()))) {
                token += static_cast<char>(input.get());
            }

            if (token == "true") return Node{true};
            if (token == "false") return Node{false};
            if (token == "null") return Node{nullptr};

            throw ParsingError(std::string("Unknown token: ") + token);
        }

        // что-то пришло не так
        throw ParsingError(std::string("Unexpected character: ") + c);
    }
    throw ParsingError("Unexpected EOF");
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit([&ctx](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            ctx.out << "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            ctx.out << (val ? "true" : "false");
        } else if constexpr (std::is_same_v<T, int>) {
            ctx.out << val;
        } else if constexpr (std::is_same_v<T, double>) {
            ctx.out << val;
        } else if constexpr (std::is_same_v<T, std::string>) {
            ctx.out << '"';
            for (char c : val) {
                switch (c) {
                    case '\n': ctx.out << "\\n"; break;
                    case '\t': ctx.out << "\\t"; break;
                    case '\r': ctx.out << "\\r"; break;
                    case '"': ctx.out << "\\\""; break;
                    case '\\': ctx.out << "\\\\"; break;
                    default: ctx.out << c;
                }
            }
            ctx.out << '"';
        } else if constexpr (std::is_same_v<T, Array>) {
            if (val.empty()) {
                ctx.out << "[]";
                return;
            }
            ctx.out << "[\n";
            auto indented = ctx.Indented();
            for (size_t i = 0; i < val.size(); ++i) {
                indented.PrintIndent();
                PrintNode(val[i], indented);
                if (i + 1 != val.size()) ctx.out << ',';
                ctx.out << '\n';
            }
            ctx.PrintIndent();
            ctx.out << ']';
        } else if constexpr (std::is_same_v<T, Dict>) {
            if (val.empty()) {
                ctx.out << "{}";
                return;
            }
            ctx.out << "{\n";
            auto indented = ctx.Indented();
            size_t i = 0;
            for (const auto& [key, value] : val) {
                indented.PrintIndent();
                ctx.out << '"';
                for (char c : key) {
                    switch (c) {
                        case '\n': ctx.out << "\\n"; break;
                        case '\t': ctx.out << "\\t"; break;
                        case '\r': ctx.out << "\\r"; break;
                        case '"': ctx.out << "\\\""; break;
                        case '\\': ctx.out << "\\\\"; break;
                        default: ctx.out << c;
                    }
                }
                ctx.out << "\": ";
                PrintNode(value, indented);
                if (++i != val.size()) ctx.out << ',';
                ctx.out << '\n';
            }
            ctx.PrintIndent();
            ctx.out << '}';
        }
    }, node.GetValue());
}

} // anonymous namespace

Node::Node(Array array) : value_(std::move(array)) {}
Node::Node(Dict map) : value_(std::move(map)) {}
Node::Node(bool value) : value_(value) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(std::string value) : value_(std::move(value)) {}
Node::Node(std::nullptr_t) : value_(nullptr) {}

bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
bool Node::IsDouble() const { return std::holds_alternative<double>(value_) || std::holds_alternative<int>(value_); }
bool Node::IsPureDouble() const { return std::holds_alternative<double>(value_); }
bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

int Node::AsInt() const {
    if (auto* p = std::get_if<int>(&value_)) return *p;
    throw std::logic_error("Not an int");
}

bool Node::AsBool() const {
    if (auto* p = std::get_if<bool>(&value_)) return *p;
    throw std::logic_error("Not a bool");
}

double Node::AsDouble() const {
    if (auto* p = std::get_if<double>(&value_)) return *p;
    if (auto* p = std::get_if<int>(&value_)) return static_cast<double>(*p);
    throw std::logic_error("Not a number");
}

const std::string& Node::AsString() const {
    if (auto* p = std::get_if<std::string>(&value_)) return *p;
    throw std::logic_error("Not a string");
}

const Array& Node::AsArray() const {
    if (auto* p = std::get_if<Array>(&value_)) return *p;
    throw std::logic_error("Not an array");
}

const Dict& Node::AsMap() const {
    if (auto* p = std::get_if<Dict>(&value_)) return *p;
    throw std::logic_error("Not a map");
}

bool Node::operator==(const Node& other) const {
    return value_ == other.value_;
}

bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

bool Node::operator==(const Array& other_array) const {
    if (const Array* this_array = std::get_if<Array>(&value_)) {
        return *this_array == other_array;
    }
    return false;
}

bool Node::operator==(const Dict& other_dict) const {
    if (const Dict* this_dict = std::get_if<Dict>(&value_)) {
        return *this_dict == other_dict;
    }
    return false;
}

Document::Document(Node root) : root_(std::move(root)) {}
const Node& Document::GetRoot() const { return root_; }

bool Document::operator==(const Document& other) const {
    return root_ == other.root_;
}

bool Document::operator!=(const Document& other) const {
    return !(*this == other);
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintContext ctx{output};
    PrintNode(doc.GetRoot(), ctx);
    output << std::endl;
}

} // namespace json