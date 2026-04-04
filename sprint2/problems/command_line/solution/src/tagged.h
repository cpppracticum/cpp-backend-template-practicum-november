#pragma once

#include <compare>
#include <functional>
#include <string>

namespace util {

template <typename Value, typename Tag>
class Tagged {
public:
    using value_type = Value;

    explicit Tagged(Value value) : value_(std::move(value)) {}

    const Value& operator*() const { return value_; }
    const Value* operator->() const { return &value_; }

    friend bool operator==(const Tagged& lhs, const Tagged& rhs) { return lhs.value_ == rhs.value_; }
    friend std::strong_ordering operator<=>(const Tagged& lhs, const Tagged& rhs) { return lhs.value_ <=> rhs.value_; }

private:
    Value value_;
};

template <typename Tagged>
struct TaggedHasher {
    size_t operator()(const Tagged& tagged) const {
        return std::hash<typename Tagged::value_type>{}(*tagged);
    }
};

}  // namespace util

namespace std {
    template <typename Value, typename Tag>
    struct hash<util::Tagged<Value, Tag>> {
        size_t operator()(const util::Tagged<Value, Tag>& tagged) const {
            return hash<Value>{}(*tagged);
        }
    };
}
