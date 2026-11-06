// 你需要提交这份代码
#pragma once

#include "visitor.h"

struct calculator : visitor {
    virtual std::any visit_num(num_node *nodes) override {
        return nodes->number;
    }
    virtual std::any visit_add(add_node *nodes) override {
        auto lval = visit(nodes->lnode);
        auto rval = visit(nodes->rnode);
        if (lval.type() == typeid(double)) {
            double lv = std::any_cast<double>(lval);
            double rv = std::any_cast<double>(rval);
            return std::any(lv + rv);
        } else{
            long long lv = std::any_cast<long long>(lval);
            long long rv = std::any_cast<long long>(rval);
            return std::any(lv + rv);
        }
    }
    virtual std::any visit_sub(sub_node *nodes) override {
        auto lval = visit(nodes->lnode);
        auto rval = visit(nodes->rnode);
        if (lval.type() == typeid(double)) {
            double lv = std::any_cast<double>(lval);
            double rv = std::any_cast<double>(rval);
            return std::any(lv - rv);
        } else{
            long long lv = std::any_cast<long long>(lval);
            long long rv = std::any_cast<long long>(rval);
            return std::any(lv - rv);
        }
    }
    virtual std::any visit_mul(mul_node *nodes) override {
        auto lval = visit(nodes->lnode);
        auto rval = visit(nodes->rnode);
        if (lval.type() == typeid(double)) {
            double lv = std::any_cast<double>(lval);
            double rv = std::any_cast<double>(rval);
            return std::any(lv * rv);
        } else{
            long long lv = std::any_cast<long long>(lval);
            long long rv = std::any_cast<long long>(rval);
            return std::any(lv * rv);
        }
    }
    virtual std::any visit_div(div_node *nodes) override {
        auto lval = visit(nodes->lnode);
        auto rval = visit(nodes->rnode);
        if (lval.type() == typeid(double)) {
            double lv = std::any_cast<double>(lval);
            double rv = std::any_cast<double>(rval);
            return std::any(lv / rv);
        } else{
            long long lv = std::any_cast<long long>(lval);
            long long rv = std::any_cast<long long>(rval);
            return std::any(lv / rv);
        }
    }
    ~calculator() override = default;
};
