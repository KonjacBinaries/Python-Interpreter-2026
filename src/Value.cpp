#include "Value.h"

#include <stdexcept>
#include <cmath>

Bool convertToBool(const Single &value) noexcept(false) {
    switch (static_cast<ValueIndex>(value.index())) {
        case ValueIndex::None :
            return false;
        case ValueIndex::Bool :
            return std::get<Bool>(value);
        case ValueIndex::Int :
            return !(std::get<Int>(value).is_zero());
        case ValueIndex::Float :
            return static_cast<bool>(std::get<Float>(value));
        case ValueIndex::Str :
            return static_cast<bool>(std::get<Str>(value).size());
        default:
            throw std::invalid_argument("[Error] In function 'convertToBool(const Single &value)' : unsupported type.");
    }
}
Str convertToStr(const Single &value) noexcept(false) {
    switch (static_cast<ValueIndex>(value.index())) {
        case ValueIndex::None :
            return std::string("None");
        case ValueIndex::Bool :
            return std::string(std::get<Bool>(value) ? "True" : "False");
        case ValueIndex::Int :
            return std::get<Int>(value).to_string();
        case ValueIndex::Float :
            return std::to_string(std::get<Float>(value));
        case ValueIndex::Str :
            return std::get<Str>(value);
        default:
            throw std::invalid_argument("[Error] In function 'convertToStr(const Single &value)' : unsupported type.");
    }
}
Float convertToFloat(const Single &value) {
    switch (static_cast<ValueIndex>(value.index())) {
        case ValueIndex::Bool :
            return static_cast<Float>(std::get<Bool>(value));
        case ValueIndex::Int :
            return std::get<Int>(value).to_double();
        case ValueIndex::Float :
            return std::get<Float>(value);
        case ValueIndex::Str :
            return std::stod(std::get<Str>(value));
        default:
            throw std::invalid_argument("[Error] In function 'convertToFloat(const Single &value)' : unsupported type.");
    }
}
auto eraseDecimal = [](std::string &&str)->std::string {
    return str.erase(str.size() - 7);
};
Int convertToInt(const Single &value) {
    std::string integer;
    switch (static_cast<ValueIndex>(value.index())) {
        case ValueIndex::Bool :
            return Int(static_cast<long long>(std::get<Bool>(value)));
        case ValueIndex::Int :
            return std::get<Int>(value);
        case ValueIndex::Float :
            // 值得注意的是，std::to_string会触发四舍五入，所以得搞一个std::trunc先截取整数部分
            return Int(eraseDecimal(std::to_string(std::trunc(std::get<Float>(value)))));
        case ValueIndex::Str :
            return Int(std::get<std::string>(value));
        default:
            throw std::invalid_argument("[Error] In function 'convertToInt(const Single &value)' : unsupported type.");
    }
}