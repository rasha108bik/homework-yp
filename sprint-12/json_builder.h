#include <utility>
#include <memory>

#include "json.h"

namespace json {
    class DictItemContext;
    class ArrayItemContext;
    class KeyItemContext;

    class Builder {
    public:
        DictItemContext& StartDict();
        Builder& EndDict();

        ArrayItemContext& StartArray();
        Builder& EndArray();

        KeyItemContext& Key(const std::string& key);
        Builder& Value(const Node& value);

        Node Build();

    private:
        Builder& AsOwner() {
            return static_cast<Builder&>(*this);
        }

        enum stage {
            START,
            PROCESS,
            FINISH,
        };

        int current_stage_ = stage::START;
        std::vector<Node> nodes_stack_;
        bool first_;
    };

    class DictItemContext {
    public:
        DictItemContext(Builder& builder) : builder_(builder) {}

        KeyItemContext& Key(const std::string& key) {
            return builder_.Key(key);
        }

        Builder& EndDict() {
            return builder_.EndDict();
        }

    private:
        Builder& builder_;
    };

    class ArrayItemContext {
    public:
        ArrayItemContext(Builder& builder) : builder_(builder) {}

        Builder& EndArray() {
            return builder_.EndArray();
        }

        ArrayItemContext& Value(const Node& value) {
            return *std::make_shared<ArrayItemContext>(builder_.Value(value));
        }

        DictItemContext& StartDict() {
            return builder_.StartDict();
        }

        ArrayItemContext& StartArray() {
            return builder_.StartArray();
        }

    private:
        Builder& builder_;
    };

    class KeyItemContext {
    public:
        KeyItemContext(Builder& builder) : builder_(builder) {}

        DictItemContext& StartDict() {
            return builder_.StartDict();
        }

        ArrayItemContext& StartArray() {
            return builder_.StartArray();
        }

        DictItemContext& Value(const Node& value) {
            return *std::make_shared<DictItemContext>(builder_.Value(value));
        }

    private:
        Builder& builder_;
    };

}