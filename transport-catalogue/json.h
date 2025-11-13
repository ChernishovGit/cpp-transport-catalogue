#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <type_traits>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node() = default;

    Node(Array array);
    Node(Dict map);
    Node(bool value);
    Node(int value);
    Node(double value);
    Node(std::string value);
    Node(std::nullptr_t);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const { return value_; }

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;
    bool operator==(const Array& other_array) const;
    bool operator==(const Dict& other_dict) const;

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;

    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;
private:
    Node root_;
};

Document Load(std::istream& input);
void Print(const Document& doc, std::ostream& output);

} // namespace json