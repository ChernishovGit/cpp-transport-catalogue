#pragma once

#include "json.h"
#include <string>
#include <vector>
#include <optional>

namespace json {

class Builder;

class KeyContext;
class DictValueContext;
class ArrayValueContext;

class Builder {
public:
    Builder();

    KeyContext Key(std::string key);
    Builder& Value(Node::Value value);
    DictValueContext StartDict();
    ArrayValueContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();

private:
    void EnsureNotBuilt();
    void EnsureCanModify();
    void EnsureCanAddValue();
    void EnsureInDict();
    void EnsureInArray();
    void EnsureKeyAllowed();

    friend class KeyContext;
    friend class DictValueContext;
    friend class ArrayValueContext;

    std::optional<Node> root_;
    std::vector<Node*> nodes_stack_;
    std::optional<std::string> pending_key_;
    bool is_built_ = false;
};

class KeyContext {
public:
    explicit KeyContext(Builder& builder);
    
    DictValueContext Value(Node::Value value);
    DictValueContext StartDict();
    ArrayValueContext StartArray();

private:
    Builder* builder_;
};

class DictValueContext {
public:
    explicit DictValueContext(Builder& builder);
    
    KeyContext Key(std::string key);
    Builder& EndDict();

private:
    Builder* builder_;
};

class ArrayValueContext {
public:
    explicit ArrayValueContext(Builder& builder);
    
    ArrayValueContext Value(Node::Value value);
    DictValueContext StartDict();
    ArrayValueContext StartArray();
    Builder& EndArray();

private:
    Builder* builder_;
};

}  // namespace json