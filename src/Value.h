#pragma once // 这是值管理头文件

#include "int2048.h"

#include <variant>
#include <vector>
#include <string>

enum class TypeID{
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

/**
 * [单值运算] 输出流
 */
std::ostream &operator<<(std::ostream &ostr, const Single &value);

/**
 * [单值运算] 加法
 */
Single operator+(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 减法
 */
Single operator-(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 乘法
 */
Single operator*(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 除法
 */
Single operator/(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 整除
 *  借用|来表示整除，因为//会被识别成注释
 */
Single operator|(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 模余
 */
Single operator%(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 一元正号
 */
Single operator+(const Single &value);

/**
 * [单值运算] 一元负号
 */
Single operator-(const Single &value);

/**
 * [单值运算] 非
 */
Bool operator not(const Single &value);

/**
 * @attention 与(and) 和 或(or) 具有短路特性，不应该在这里实现，这部分功能会在visitor里实现
 */

/**
 * [单值运算] 相等
 */
Bool operator==(const Single &value_l, const Single &value_r);

/**
 * [单值运算] 不等
 */
Bool operator!=(const Single &value_l, const Single &value_r);

/**
 * [单值运算]大于等于
 */
Bool operator>=(const Single &value_l, const Single &value_r);

/**
 * [单值运算]小于等于
 */
Bool operator<=(const Single &value_l, const Single &value_r);

/**
 * [单值运算]大于
 */
Bool operator>(const Single &value_l, const Single &value_r);

/**
 * [单值运算]小于
 */
Bool operator<(const Single &value_l, const Single &value_r);