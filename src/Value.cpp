#include "Value.h"

#include <stdexcept>
#include <cmath>

auto typeID = [](const Single &value)->TypeID {
    return static_cast<TypeID>(value.index());
};

template <TypeID type_id>
bool checkType(const Single &value) {
    return typeID(value) == type_id;
}

Bool convertToBool(const Single &value) noexcept(false) {
    switch (typeID(value)) {
        case TypeID::None :
            return false;
        case TypeID::Bool :
            return std::get<Bool>(value);
        case TypeID::Int :
            return !(std::get<Int>(value).is_zero());
        case TypeID::Float :
            return static_cast<bool>(std::get<Float>(value));
        case TypeID::Str :
            return static_cast<bool>(std::get<Str>(value).size());
        default:
            throw std::invalid_argument("[Error] In function 'convertToBool(const Single &value)' : unsupported type.");
    }
}

Str convertToStr(const Single &value) noexcept(false) {
    switch (typeID(value)) {
        case TypeID::None :
            return std::string("None");
        case TypeID::Bool :
            return std::string(std::get<Bool>(value) ? "True" : "False");
        case TypeID::Int :
            return std::get<Int>(value).to_string();
        case TypeID::Float :
            return std::to_string(std::get<Float>(value));
        case TypeID::Str :
            return std::get<Str>(value);
        default:
            throw std::invalid_argument("[Error] In function 'convertToStr(const Single &value)' : unsupported type.");
    }
}

Float convertToFloat(const Single &value) {
    switch (typeID(value)) {
        case TypeID::Bool :
            return static_cast<Float>(std::get<Bool>(value));
        case TypeID::Int :
            return std::get<Int>(value).to_double();
        case TypeID::Float :
            return std::get<Float>(value);
        case TypeID::Str :
            return std::stod(std::get<Str>(value));
        default:
            throw std::invalid_argument("[Error] In function 'convertToFloat(const Single &value)' : unsupported type.");
    }
}

auto eraseDecimal = [](std::string &&str)->std::string {
    return str.erase(str.size() - 7);
};

Int convertToInt(const Single &value) {
    switch (typeID(value)) {
        case TypeID::Bool :
            return Int(static_cast<long long>(std::get<Bool>(value)));
        case TypeID::Int :
            return std::get<Int>(value);
        case TypeID::Float :
            // 值得注意的是，std::to_string会触发四舍五入，所以得搞一个std::trunc先截取整数部分
            return Int(eraseDecimal(std::to_string(std::trunc(std::get<Float>(value)))));
        case TypeID::Str :
            return Int(std::get<std::string>(value));
        default:
            throw std::invalid_argument("[Error] In function 'convertToInt(const Single &value)' : unsupported type.");
    }
}

std::ostream &operator<<(std::ostream &ostr, const Single &value) {
    switch (typeID(value)) {
        case TypeID::None :
            return ostr << std::string("None");
        case TypeID::Bool :
            return ostr << std::string(std::get<Bool>(value) ? "True" : "False");
        case TypeID::Int :
            return ostr << std::get<Int>(value);
        case TypeID::Float :
            return ostr << std::to_string(std::get<Float>(value));
        case TypeID::Str :
            return ostr << std::get<Str>(value);
        default:
            throw std::invalid_argument("[Error] In function '&operator<<(std::ostream &ostr, const Single &value)' : unsupported type.");
    }
}

Single operator+(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator+(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除Str和非Str相加
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator+(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 处理Str和Str相加
    if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) + std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) + convertToFloat(value_r);
    }

    // 剩余情况，所有值只在Bool和Int之间
    return convertToInt(value_l) + convertToInt(value_r);
}

Single operator-(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator-(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除所有Str
    if (checkType<TypeID::Str>(value_l) || checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator-(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) - convertToFloat(value_r);
    }

    // 剩余情况，所有值只在Bool和Int之间
    return convertToInt(value_l) - convertToInt(value_r);
}

Single operator*(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator*(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除Str * Str
    if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator*(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // Str * 别的
    if (checkType<TypeID::Str>(value_l)) {
        // 排除Str * Float
        if (checkType<TypeID::Float>(value_r)) {
            throw std::invalid_argument("[Error] In function 'operator*(const Single &value_l, const Single &value_r)' : unsupported combination.");
        }
        Int r = convertToInt(value_r);
        if (r.is_negative() || r.is_zero()) {
            return Str("");
        }
        std::string value_str = std::get<Str>(value_l);
        if (value_str.empty()) {
            return Str("");
        }
        std::string res = value_str;
        while (!(r = r - 1).is_zero()) {
            res = res + value_str;
        }
        return res;
    }

    // 别的 * Str
    if (checkType<TypeID::Str>(value_r)) {
        // 排除Str * Float
        if (checkType<TypeID::Float>(value_l)) {
            throw std::invalid_argument("[Error] In function 'operator*(const Single &value_l, const Single &value_r)' : unsupported combination.");
        }
        Int l = convertToInt(value_l);
        if (l.is_negative() || l.is_zero()) {
            return Str("");
        }
        std::string value_str = std::get<Str>(value_r);
        if (value_str.empty()) {
            return Str("");
        }
        std::string res = value_str;
        while (!(l = l - 1).is_zero()) {
            res = res + value_str;
        }
        return res;
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) * convertToFloat(value_r);
    }

    // 剩余情况，所有值只在Bool和Int之间
    return convertToInt(value_l) * convertToInt(value_r);
}

Single operator/(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator/(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除所有Str
    if (checkType<TypeID::Str>(value_l) || checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator/(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除除数是0的情况
    if ((checkType<TypeID::Bool>(value_r) && std::get<Bool>(value_r) == false)
        || (checkType<TypeID::Int>(value_r) && std::get<Int>(value_r).is_zero())
        || (checkType<TypeID::Float>(value_r) && std::get<Float>(value_r) == 0.0)) {
        throw std::runtime_error("[Error] In function 'operator/(const Single &value_l, const Single &value_r)' : the divisor cannot be zero.");
    }

    return convertToFloat(value_l) / convertToFloat(value_r);
}

Single operator|(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator|(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除所有Str
    if (checkType<TypeID::Str>(value_l) || checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator|(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除除数是0的情况
    if ((checkType<TypeID::Bool>(value_r) && std::get<Bool>(value_r) == false)
        || (checkType<TypeID::Int>(value_r) && std::get<Int>(value_r).is_zero())
        || (checkType<TypeID::Float>(value_r) && std::get<Float>(value_r) == 0.0)) {
        throw std::runtime_error("[Error] In function 'operator|(const Single &value_l, const Single &value_r)' : the divisor cannot be zero.");
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return std::floor(convertToFloat(value_l) / convertToFloat(value_r));
    }

    // 剩余情况，所有值只在Bool和Int之间
    return convertToInt(value_l) / convertToInt(value_r);
}

Single operator%(const Single &value_l, const Single &value_r) {
    // 排除所有None
    if (checkType<TypeID::None>(value_l) || checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator%(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除所有Str
    if (checkType<TypeID::Str>(value_l) || checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator%(const Single &value_l, const Single &value_r)' : unsupported combination.");
    }

    // 排除除数是0的情况
    if ((checkType<TypeID::Bool>(value_r) && std::get<Bool>(value_r) == false)
        || (checkType<TypeID::Int>(value_r) && std::get<Int>(value_r).is_zero())
        || (checkType<TypeID::Float>(value_r) && std::get<Float>(value_r) == 0.0)) {
        throw std::runtime_error("[Error] In function 'operator%(const Single &value_l, const Single &value_r)' : the divisor cannot be zero.");
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        Float l_val = convertToFloat(value_l), r_val = convertToFloat(value_r);
        // 直接a % b = a - (a // b) * b会精度爆炸，std::fmod(a, b)是更优选择，不过它向0取整
        Float res = std::fmod(l_val, r_val);
        // 最终，A / B = Q + R / B，下取整要求R / B >= 0，也就是B和R同号
        if (res == 0.0) {
            return std::signbit(r_val) ? -0.0 : 0.0;
        }
        if (std::signbit(res) ^ std::signbit(r_val)) {
            res += r_val;
        }
        return res; 
    }

    // 剩余情况，所有值只在Bool和Int之间
    return convertToInt(value_l) % convertToInt(value_r);
}

Single operator+(const Single &value) {
    // 排除None
    if (checkType<TypeID::None>(value)) {
        throw std::invalid_argument("[Error] In function 'operator+(const Single &value_l, const Single &value_r)' : unsupported type.");
    }

    // 排除Str
    if (checkType<TypeID::Str>(value)) {
        throw std::invalid_argument("[Error] In function 'operator+(const Single &value_l, const Single &value_r)' : unsupported type.");
    }

    // 处理Float
    if (checkType<TypeID::Float>(value)) {
        return std::get<Float>(value);
    }

    return convertToInt(value);
}

Single operator-(const Single &value) {
    // 排除None
    if (checkType<TypeID::None>(value)) {
        throw std::invalid_argument("[Error] In function 'operator-(const Single &value_l, const Single &value_r)' : unsupported type.");
    }

    // 排除Str
    if (checkType<TypeID::Str>(value)) {
        throw std::invalid_argument("[Error] In function 'operator-(const Single &value_l, const Single &value_r)' : unsupported type.");
    }

    // 处理Float
    if (checkType<TypeID::Float>(value)) {
        return -1.0 * std::get<Float>(value);
    }
    
    Int res = convertToInt(value);
    res.to_negative();
    return res;
}

Bool operator not(const Single &value) {
    try {
        return convertToBool(value) ^ true;
    } catch (...) {
        throw std::invalid_argument("[Error] In function 'operator not(const Single &value)' : unsupported type.");
    }
}

Bool operator==(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        return false;
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return true;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        return false;
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) == std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) == convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) == convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) == std::get<Bool>(value_r);
}

Bool operator!=(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        return false;
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return false;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        return false;
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) != std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) != convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) != convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) != std::get<Bool>(value_r);
}

Bool operator>=(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator>=(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return true;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator>=(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) >= std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) >= convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) >= convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) >= std::get<Bool>(value_r);
}

Bool operator<=(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator<=(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return true;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator<=(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) <= std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) <= convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) <= convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) <= std::get<Bool>(value_r);
}

Bool operator>(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator>(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return false;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator>(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) > std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) > convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) > convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) > std::get<Bool>(value_r);
}

Bool operator<(const Single &value_l, const Single &value_r) {
    // 处理None
    if (checkType<TypeID::None>(value_l) ^ checkType<TypeID::None>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator<(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::None>(value_l) && checkType<TypeID::None>(value_r)) {
        return false;
    }

    // 处理Str
    if (checkType<TypeID::Str>(value_l) ^ checkType<TypeID::Str>(value_r)) {
        throw std::invalid_argument("[Error] In function 'operator<(const Single &value_l, const Single &value_r)' : unsupported combination.");
    } else if (checkType<TypeID::Str>(value_l) && checkType<TypeID::Str>(value_r)) {
        return std::get<Str>(value_l) < std::get<Str>(value_r);
    }

    // 处理存在Float的情况
    if (checkType<TypeID::Float>(value_l) || checkType<TypeID::Float>(value_r)) {
        return convertToFloat(value_l) < convertToFloat(value_r);
    }

    // 处理存在Int的情况
    if (checkType<TypeID::Int>(value_l) || checkType<TypeID::Int>(value_r)) {
        return convertToInt(value_l) < convertToInt(value_r);
    }

    // 处理Bool
    return std::get<Bool>(value_l) < std::get<Bool>(value_r);
}