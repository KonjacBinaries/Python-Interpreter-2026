#pragma once // 这是值管理头文件

#include "int2048.h"

#include <variant>
#include <vector>
#include <string>

enum class ValueIndex{
    None = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    Str = 4
};
using None = std::monostate;
using Bool = bool;
using Int = sjtu::int2048;
using Float = double;
using Str = std::string;

using Single = std::variant<None,
                            Bool,
                            Int,
                            Float,
                            Str>; // 单值类型
using Tuple = std::vector<Single>; // 元组类型
using Value = std::variant<Single, Tuple>; // 值类型

/**
 * [值转换] 把受支持的值转换为Bool
 *  支持 None, Bool, Int, Float, Str
 */
Bool convertToBool(const Single &value);

/**
 * [值转换] 把受支持的值转换为Str
 *  支持 None, Bool, Int, Float, Str
 */
Str convertToStr(const Single &value);

/**
 * [值转换] 把受支持的值转换为Float
 *  支持 Bool, Int, Float, Str
 */
Float convertToFloat(const Single &value);

/**
 * [值转换] 把受支持的值转换为Int
 *  支持 Bool, Int, Float, Str
 */
Int convertToInt(const Single &value);
