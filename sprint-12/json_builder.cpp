#include "json_builder.h"

using namespace json;
using namespace std::literals;

DictItemContext& Builder::StartDict() {
    switch (current_stage_) {
        case stage::START:
            nodes_stack_.push_back(Dict{});
            current_stage_ = stage::PROCESS;
            break;

        case stage::PROCESS:
            if (nodes_stack_.back().IsArray() || nodes_stack_.back().IsString()) {
                nodes_stack_.push_back(Dict{});
            } else {
                throw std::logic_error("StartDict stage::PROCESS failed back type"s);
            }
            break;

        case stage::FINISH:
            throw std::logic_error("StartDict stage::FINISH"s);

        default:
            throw std::logic_error("StartDict undefined state"s);
    }

    return *std::make_shared<DictItemContext>(AsOwner());
}

Builder &Builder::EndDict() {
    switch (current_stage_) {
        case stage::START:
            throw std::logic_error("EndDict not created"s);

        case stage::PROCESS:
            if (nodes_stack_.back().IsDict()) {
                if (nodes_stack_.size() == 1u) {
                    current_stage_ = stage::FINISH;
                } else {
                    Dict last_dict = nodes_stack_.back().AsDict();
                    nodes_stack_.pop_back();

                    if (nodes_stack_.back().IsArray()) {
                        Array arr = nodes_stack_.back().AsArray();
                        arr.push_back(last_dict);
                        nodes_stack_.back() = arr;
                    } else if (nodes_stack_.back().IsString() && nodes_stack_[nodes_stack_.size() - 2u].IsDict()) {
                        std::string key = nodes_stack_.back().AsString();
                        nodes_stack_.pop_back();

                        Dict dict = nodes_stack_.back().AsDict();
                        dict[key] = last_dict;

                        nodes_stack_.back() = dict;
                    } else {
                        throw std::logic_error("EndDict stage::PROCESS failed type"s);
                    }
                }
            } else {
                throw std::logic_error("EndDict is not dict"s);
            }
            break;

        case stage::FINISH:
            throw std::logic_error("EndDict stage::FINISH"s);

        default:
            throw std::logic_error("EndDict undefined state"s);
    }

    return AsOwner();
}

ArrayItemContext &Builder::StartArray() {
    switch (current_stage_) {
        case stage::START:
            nodes_stack_.push_back(Array{});
            current_stage_ = stage::PROCESS;
            break;

        case stage::PROCESS:
            if (nodes_stack_.back().IsDict()) {
                throw std::logic_error("StartArray stage::PROCESS failed back type"s);
            }
            nodes_stack_.push_back(Array{});
            break;

        case stage::FINISH:
            throw std::logic_error("StartArray stage: FINISH"s);

        default:
            throw std::logic_error("StartArray undefined state"s);
    }

    return *std::make_shared<ArrayItemContext>(AsOwner());;
}

Builder &Builder::EndArray() {
    switch (current_stage_) {
        case stage::START:
            throw std::logic_error("EndArray not created"s);

        case stage::PROCESS:
            if (nodes_stack_.back().IsArray()) {
                if (nodes_stack_.size() == 1u) {
                    current_stage_ = stage::FINISH;
                } else {
                    auto last_arr = nodes_stack_.back().AsArray();
                    nodes_stack_.pop_back();

                    if (nodes_stack_.back().IsString() && nodes_stack_[nodes_stack_.size() - 2u].IsDict()) {
                        std::string key = nodes_stack_.back().AsString();
                        nodes_stack_.pop_back();

                        Dict dict = nodes_stack_.back().AsDict();
                        dict[key] = last_arr;

                        nodes_stack_.back() = dict;
                    } else if (nodes_stack_.back().IsArray()) {
                        Array last_last_arr = nodes_stack_.back().AsArray();
                        last_last_arr.push_back(last_arr);

                        nodes_stack_.back() = last_last_arr;
                    } else {
                        throw std::logic_error("EndArray stage::PROCESS failed type"s);
                    }
                }
            } else {
                throw std::logic_error("EndArray is not array"s);
            }
            break;

        case stage::FINISH:
            throw std::logic_error("EndArray stage::FINISH"s);

        default:
            throw std::logic_error("EndArray undefined state"s);
    }

    return AsOwner();
}

KeyItemContext& Builder::Key(const std::string &key) {
    if (nodes_stack_.back().IsDict() && current_stage_ != stage::FINISH) {
        nodes_stack_.emplace_back(key);
    } else {
        throw std::logic_error("add key: dict not created"s);
    }

    return *std::make_shared<KeyItemContext>(AsOwner());
}

Builder &Builder::Value(const Node &value) {
    if (!nodes_stack_.empty()) {
        if (nodes_stack_.back().IsString()) {
            std::string key = nodes_stack_.back().AsString();
            nodes_stack_.pop_back();

            Dict dict = nodes_stack_.back().AsDict();
            dict.insert({key, value});
            nodes_stack_.back() = dict;
        } else if (nodes_stack_.back().IsArray()) {
            Array arr = nodes_stack_.back().AsArray();
            arr.push_back(value);
            nodes_stack_.back() = arr;
        } else {
            throw std::logic_error("call Value failed"s);
        }
    } else {
        if (!first_) {
            nodes_stack_.emplace_back(value);
            first_ = true;
            current_stage_ = stage::FINISH;
        }
    }

    return AsOwner();
}

Node Builder::Build() {
    if (nodes_stack_.size() != 1u || current_stage_ != stage::FINISH) {
        throw std::logic_error("object is not completed"s);
    }
    return nodes_stack_.back();
}