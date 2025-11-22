#include "json_builder.h"
#include <stdexcept>
#include <string_view>

using namespace std::literals;

namespace json {

namespace {

Node ValueToNode(Node::Value&& value) {
    return std::visit([](auto&& arg) -> Node {
        return Node(std::forward<decltype(arg)>(arg));
    }, std::move(value));
}

}  // namespace

Builder::Builder() = default;

void Builder::EnsureNotBuilt() {
    if (is_built_) {
        throw std::logic_error("Builder is already built."s);
    }
}

void Builder::EnsureCanModify() {
    EnsureNotBuilt();
    if (nodes_stack_.empty() && root_.has_value()) {
        throw std::logic_error("Cannot modify: root value is already set."s);
    }
}

void Builder::EnsureCanAddValue() {
    if (!nodes_stack_.empty()) {
        if (nodes_stack_.back()->IsMap()) {
            if (!pending_key_.has_value()) {
                throw std::logic_error("Key is required before value in dictionary."s);
            }
        }
    }
}

void Builder::EnsureInDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
        throw std::logic_error("EndDict called not inside a dictionary."s);
    }
}

void Builder::EnsureInArray() {  // ← Обязательно должен быть!
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("EndArray called not inside an array."s);
    }
}

void Builder::EnsureKeyAllowed() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap() || pending_key_.has_value()) {
        throw std::logic_error("Key can be set only inside a dictionary and only once before a value."s);
    }
}

KeyContext Builder::Key(std::string key) {
    EnsureCanModify();
    EnsureKeyAllowed();
    pending_key_ = std::move(key);
    return KeyContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    EnsureCanModify();
    EnsureCanAddValue();

    Node node = ValueToNode(std::move(value));

    if (nodes_stack_.empty()) {
        root_ = std::move(node);
    } else {
        Node* parent = nodes_stack_.back();
        if (parent->IsMap()) {
            auto& dict = const_cast<Dict&>(parent->AsMap());
            dict.emplace(std::move(*pending_key_), std::move(node));
            pending_key_.reset();
        } else if (parent->IsArray()) {
            auto& arr = const_cast<Array&>(parent->AsArray());
            arr.emplace_back(std::move(node));
        }
    }
    return *this;
}

DictValueContext Builder::StartDict() {
    EnsureCanModify();
    EnsureCanAddValue();

    if (nodes_stack_.empty()) {
        root_.emplace(Dict{});
        nodes_stack_.push_back(&*root_);
    } else {
        Node* parent = nodes_stack_.back();
        if (parent->IsMap()) {
            auto& dict = const_cast<Dict&>(parent->AsMap());
            dict.emplace(std::move(*pending_key_), Dict{});
            pending_key_.reset();
            nodes_stack_.push_back(&dict.rbegin()->second);
        } else if (parent->IsArray()) {
            auto& arr = const_cast<Array&>(parent->AsArray());
            arr.emplace_back(Dict{});
            nodes_stack_.push_back(&arr.back());
        }
    }
    return DictValueContext(*this);
}

ArrayValueContext Builder::StartArray() {
    EnsureCanModify();
    EnsureCanAddValue();

    if (nodes_stack_.empty()) {
        root_.emplace(Array{});
        nodes_stack_.push_back(&*root_);
    } else {
        Node* parent = nodes_stack_.back();
        if (parent->IsMap()) {
            auto& dict = const_cast<Dict&>(parent->AsMap());
            dict.emplace(std::move(*pending_key_), Array{});
            pending_key_.reset();
            nodes_stack_.push_back(&dict.rbegin()->second);
        } else if (parent->IsArray()) {
            auto& arr = const_cast<Array&>(parent->AsArray());
            arr.emplace_back(Array{});
            nodes_stack_.push_back(&arr.back());
        }
    }
    return ArrayValueContext(*this);
}

Builder& Builder::EndDict() {
    EnsureCanModify();
    EnsureInDict();
    nodes_stack_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    EnsureCanModify();
    EnsureInArray();  // ← вызывает EnsureInArray
    nodes_stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    if (is_built_) {
        throw std::logic_error("Builder is already built."s);
    }
    if (!root_.has_value()) {
        throw std::logic_error("Build called with no value specified."s);
    }
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Build called with unclosed containers."s);
    }
    is_built_ = true;
    return std::move(*root_);
}

// KeyContext

KeyContext::KeyContext(Builder& builder) : builder_(&builder) {}

DictValueContext KeyContext::Value(Node::Value value) {
    builder_->Value(std::move(value));
    return DictValueContext(*builder_);
}

DictValueContext KeyContext::StartDict() {
    return builder_->StartDict();
}

ArrayValueContext KeyContext::StartArray() {
    return builder_->StartArray();
}

// DictValueContext

DictValueContext::DictValueContext(Builder& builder) : builder_(&builder) {}

KeyContext DictValueContext::Key(std::string key) {
    return builder_->Key(std::move(key));
}

Builder& DictValueContext::EndDict() {
    return builder_->EndDict();
}

// ArrayValueContext

ArrayValueContext::ArrayValueContext(Builder& builder) : builder_(&builder) {}

ArrayValueContext ArrayValueContext::Value(Node::Value value) {
    builder_->Value(std::move(value));
    return *this;
}

DictValueContext ArrayValueContext::StartDict() {
    return builder_->StartDict();
}

ArrayValueContext ArrayValueContext::StartArray() {
    return builder_->StartArray();
}

Builder& ArrayValueContext::EndArray() {
    return builder_->EndArray();
}

}  // namespace json