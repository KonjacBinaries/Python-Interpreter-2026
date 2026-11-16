#include "Evalvisitor.h"
#include "Python3Parser.h"

#include <any>
#include <stdexcept>
#include <utility>

Single EvalVisitor::readVariable(const std::string &variable_name) const {
    // 先查找局部作用域变量
    if (!locals_.empty()) {
        auto it = locals_.top().find(variable_name);
        if (it != locals_.top().end()) {
            return it->second;
        }
    }

    // 再查找全局作用域变量
    auto it = globals_.find(variable_name);
    if (it != globals_.end()) {
        return it->second;
    }

    throw std::out_of_range("[Error] In function 'EvalVisitor::readVariable(const std::string &variable_name) const' : no such variable.");
}

void EvalVisitor::writeVariable(const std::string &variable_name, const Single &value) {
    if (!locals_.empty()) {
        auto it = locals_.top().find(variable_name);
        if (it != locals_.top().end()) {
            it->second = value;
            return;
        }
        it = globals_.find(variable_name);
        if (it != globals_.end()) {
            it->second = value;
            return;
        }
        locals_.top().emplace(variable_name, value);
        return;
    }
    auto it = globals_.find(variable_name);
    if (it != globals_.end()) {
        it->second = value;
    } else {
        globals_.emplace(variable_name, value);
    }
}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    for (auto *child : ctx->children) {
        if (auto *stmt = dynamic_cast<Python3Parser::StmtContext *>(child)) {
            visit(stmt);
        }
    }
    return std::monostate{};
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->simple_stmt() != nullptr) {
        return visit(ctx->simple_stmt());
    }
    if (ctx->compound_stmt() != nullptr) {
        return visit(ctx->compound_stmt());
    }
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx)' : invalid stmt.");
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visit(ctx->small_stmt());
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt() != nullptr) {
        return visit(ctx->expr_stmt());
    }
    if (ctx->flow_stmt() != nullptr) {
        return visit(ctx->flow_stmt());
    }
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx)' : invalid small_stmt.");
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    if (ctx->augassign() != nullptr) {
        if (ctx->testlist(0)->COMMA().empty()) { // 非元组
            auto *test = ctx->testlist(0)->test(0);
            Single LHS = std::get<Single>(std::any_cast<Value>(visit(ctx->testlist(0))));
            Single RHS = std::get<Single>(std::any_cast<Value>(visit(ctx->testlist(1))));  
            if (ctx->augassign()->ADD_ASSIGN() != nullptr) {
                LHS = LHS + RHS;
            } else if (ctx->augassign()->SUB_ASSIGN() != nullptr) {
                LHS = LHS - RHS;
            } else if (ctx->augassign()->MULT_ASSIGN() != nullptr) {
                LHS = LHS * RHS;
            } else if (ctx->augassign()->DIV_ASSIGN() != nullptr) {
                LHS = LHS / RHS;
            } else if (ctx->augassign()->IDIV_ASSIGN() != nullptr) {
                LHS = LHS | RHS;
            } else if (ctx->augassign()->MOD_ASSIGN() != nullptr) {
                LHS = LHS % RHS;
            }
            writeVariable(test->getText(), LHS);
            return Result{FlowControl::Normal, Value{Single{LHS}}};
        } else { // 元组
            Tuple XHS{};
            Tuple FHS{};
            bool rhs_is_function = false;
            if (ctx->testlist(1)->COMMA().empty()) { // 等号右侧是返回值元组
                rhs_is_function = true;
                FHS = std::get<Tuple>(std::any_cast<Value>(visit(ctx->testlist(1)->test(0))));
            } 
            int pointer = 0;
            for (auto *child : ctx->testlist(0)->children) {
                if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
                    Single LHS = readVariable(test->getText()); // 读取变量
                    Single RHS{}, XVAL{};
                    if (rhs_is_function) {
                        RHS = FHS[pointer];
                    } else {
                        RHS = std::get<Single>(std::any_cast<Value>(visit(ctx->testlist(1)->test(pointer))));
                    }
                    if (ctx->augassign()->ADD_ASSIGN() != nullptr) {
                        XVAL = LHS + RHS;
                    } else if (ctx->augassign()->SUB_ASSIGN() != nullptr) {
                        XVAL = LHS - RHS;
                    } else if (ctx->augassign()->MULT_ASSIGN() != nullptr) {
                        XVAL = LHS * RHS;
                    } else if (ctx->augassign()->DIV_ASSIGN() != nullptr) {
                        XVAL = LHS / RHS;
                    } else if (ctx->augassign()->IDIV_ASSIGN() != nullptr) {
                        XVAL = LHS | RHS;
                    } else if (ctx->augassign()->MOD_ASSIGN() != nullptr) {
                        XVAL = LHS % RHS;
                    }
                    XHS.push_back(XVAL);
                    ++pointer;
                }
            }
            pointer = 0;
            for (auto *child : ctx->testlist(0)->children) {
                if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
                    writeVariable(test->getText(), XHS[pointer]);
                    ++pointer;
                }
            }
            return Result{FlowControl::Normal, Value{Tuple{XHS}}};
        }
    } // 增量赋值模式

    if (ctx->ASSIGN().empty()) {
        return Result{FlowControl::Normal, std::any_cast<Value>(visit(ctx->testlist(0)))};
    } // 表达式语句

    int variablelist_count = ctx->testlist().size() - 1; // 最后一个是求值的
    int rhs_list = ctx->testlist().size() - 1; // 这是RHS对应的testlist
    Value R_value = std::any_cast<Value>(visit(ctx->testlist(rhs_list)));
    if (ctx->testlist(0)->COMMA().empty()) { // 单值
        Single temp_value = std::get<Single>(R_value);
        for (int i = variablelist_count - 1; i >= 0; --i) {
            writeVariable(ctx->testlist(i)->test(0)->getText(), temp_value);
        }
        return Result{FlowControl::Normal, Value{Single{temp_value}}};
    } else { // 元组
        Tuple temp_value = std::get<Tuple>(R_value);
        for (int i = variablelist_count - 1; i >= 0; --i) {
            int pointer = 0;
            for (auto *child : ctx->testlist(i)->children) {
                if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
                    writeVariable(test->getText(), temp_value[pointer]);
                    ++pointer;
                }
            }
        }
        return Result{FlowControl::Normal, Value{Tuple{temp_value}}};
    } // 赋值
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    if (ctx->break_stmt() != nullptr) {
        return visit(ctx->break_stmt());
    }
    if (ctx->continue_stmt() != nullptr) {
        return visit(ctx->continue_stmt());
    }
    return visit(ctx->return_stmt());
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    return Result{FlowControl::Break, Value{Single{None{}}}};
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    return Result{FlowControl::Continue, Value{Single{None{}}}};
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    if (ctx->testlist() != nullptr) {
        return Result{FlowControl::Return, std::any_cast<Value>(visit(ctx->testlist()))};
    }
    return Result{FlowControl::Return, Value{Single{None{}}}};
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    if (ctx->if_stmt() != nullptr) {
        return visit(ctx->if_stmt());
    }
    if (ctx->while_stmt() != nullptr) {
        return visit(ctx->while_stmt());
    }
    return visit(ctx->funcdef());
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    Bool test_result = false;
    for (auto *child : ctx->children) {
        if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
            test_result = convertToBool(std::get<Single>(std::any_cast<Value>(visit(test)))); // 判定
        } else if (auto *suite = dynamic_cast<Python3Parser::SuiteContext *>(child)) {
            if (test_result) {
                return visit(suite);
            } // 判定通过
        } else if (auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
            auto terminal_type = terminal->getSymbol()->getType();
            if (terminal_type == Python3Parser::IF) {
                test_result = false; // 重置判定结果，准备开始判定
            } else if (terminal_type == Python3Parser::ELIF) {
                test_result = false; // 重置判定结果，准备开始判定
            } else if (terminal_type == Python3Parser::ELSE) {
                test_result = true; // 无条件成立
            }
        }
    }
    return Result{FlowControl::Normal, Value{Single{None{}}}}; // 无有效判定
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    auto *test = ctx->test();
    auto *suite = ctx->suite();
    while (convertToBool(std::get<Single>(std::any_cast<Value>(visit(test))))) { // 判定
        Result res = std::any_cast<Result>(visit(suite));
        if (res.flow_status_ == FlowControl::Break) {
            break;
        } else if (res.flow_status_ == FlowControl::Return) {
            return res; // return需要交给上级处理
        }
        // continue 和 normal对于 while 没有作用，那是 suite 内部的事
    }
    return Result{FlowControl::Normal, Value{Single{None{}}}};
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    Result res{};
    if (ctx->simple_stmt() != nullptr) {
        res = std::any_cast<Result>(visit(ctx->simple_stmt()));
        if (res.flow_status_ != FlowControl::Normal) {
            return res;
        } // suite的截止条件非常多，除了Normal都要截止
    }
    for (auto *i : ctx->stmt()) {
        res = std::any_cast<Result>(visit(i));
        if (res.flow_status_ != FlowControl::Normal) {
            return res;
        } // suite的截止条件非常多，除了Normal都要截止
    }
    return Result{FlowControl::Normal, Value{Single{None{}}}};
}

// 弃用
std::any EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx) {
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx)' : this function is aborted.");
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    auto info = std::any_cast<std::pair<Scope, std::vector<std::string> > >(visit(ctx->parameters()));
    functions_[ctx->NAME()->getText()] = std::make_tuple(info.first, info.second, ctx->suite());
    return Result{};
}

std::any EvalVisitor::visitParameters(Python3Parser::ParametersContext *ctx) {
    if (ctx->typedargslist() == nullptr) {
        return std::make_pair(Scope{}, std::vector<std::string>{}); // 空
    }
    // 任务: 求出函数的初始Scope以及参数名字。注意，这里可以遮蔽全局变量，不能用writeVariable
    return visit(ctx->typedargslist());
}

std::any EvalVisitor::visitTypedargslist(Python3Parser::TypedargslistContext *ctx) {
    // 任务: 求出函数的初始Scope以及参数名字。注意，这里可以遮蔽全局变量，不能用writeVariable
    auto res = std::make_pair(Scope{}, std::vector<std::string>{}); // 创建
    std::string varname{};
    for (auto *child : ctx->children) {
        if (auto *tfpdef = dynamic_cast<Python3Parser::TfpdefContext *>(child)) { // 如果不是nullptr，就是testlist
            varname = std::any_cast<std::string>(visit(tfpdef)); // 获取变量名字
            res.first.emplace(varname, Single{None{}}); // 创建变量
            res.second.push_back(varname);
        } else if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
            // 如果遇到test，肯定是给上一个变量上默认值的
            res.first[varname] = std::get<Single>(std::any_cast<Value>(visit(test))); // 赋值
        }
    }
    return res;
}

std::any EvalVisitor::visitTfpdef(Python3Parser::TfpdefContext *ctx) {
    return ctx->NAME()->getText();
}

// 此函数只能用于获得值，不负责获取变量名
std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return visit(ctx->or_test());
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    if (ctx->OR().empty()) {
        return visit(ctx->and_test(0));
    }
    Bool temp = false;
    for (auto *child : ctx->children) {
        if (auto *and_test = dynamic_cast<Python3Parser::And_testContext *>(child)) {
            temp |= convertToBool(std::get<Single>(std::any_cast<Value>(visit(and_test))));
            if (temp) {
                return Value{Single{Bool{true}}}; // 短路
            }
        }
    }
    return Value{Single{Bool{false}}};
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    if (ctx->AND().empty()) {
        return visit(ctx->not_test(0));
    }
    Bool temp = true;
    for (auto *child : ctx->children) {
        if (auto *not_test = dynamic_cast<Python3Parser::Not_testContext *>(child)) {
            temp &= convertToBool(std::get<Single>(std::any_cast<Value>(visit(not_test))));
            if (!temp) {
                return Value{Single{Bool{false}}}; // 短路
            }
        }
    }
    return Value{Single{Bool{true}}};
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->NOT() == nullptr) {
        return visit(ctx->comparison());
    }
    Bool temp = convertToBool(std::get<Single>(std::any_cast<Value>(visit(ctx->not_test()))));
    temp ^= true;
    return Value{Single{Bool{temp}}};
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    if (ctx->comp_op().empty()) {
        return visit(ctx->arith_expr(0));
    }
    bool no_firstval = true;
    Bool res = true;
    Python3Parser::Comp_opContext *op;
    Single val[2] = {{}, {}};
    bool lastval = false, thisval = true;
    for (auto *child : ctx->children) {
        if (auto *arith_expr = dynamic_cast<Python3Parser::Arith_exprContext *>(child)) {
            if (no_firstval) {
                no_firstval = false;
                val[thisval] = std::get<Single>(std::any_cast<Value>(visit(arith_expr)));
            } else {
                thisval ^= true; lastval ^= true;
                val[thisval] = std::get<Single>(std::any_cast<Value>(visit(arith_expr)));
                bool thisres = false;
                if (op->LESS_THAN() != nullptr) {
                    thisres = (val[lastval] < val[thisval]);
                } else if (op->GREATER_THAN() != nullptr) {
                    thisres = (val[lastval] > val[thisval]);
                } else if (op->EQUALS() != nullptr) {
                    thisres = (val[lastval] == val[thisval]);
                } else if (op->GT_EQ() != nullptr) {
                    thisres = (val[lastval] >= val[thisval]);
                } else if (op->LT_EQ() != nullptr) {
                    thisres = (val[lastval] <= val[thisval]);
                } else if (op->NOT_EQ_2() != nullptr) {
                    thisres = (val[lastval] != val[thisval]);
                }
                res &= thisres;
                if (!res) {
                    return Value{Single{Bool{false}}}; // 短路
                }
            }
        } else {
            op = dynamic_cast<Python3Parser::Comp_opContext *>(child);
        }
    }
    return Value{Single{Bool{true}}};
}

// 弃用
std::any EvalVisitor::visitComp_op(Python3Parser::Comp_opContext *ctx) {
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitComp_op(Python3Parser::Comp_opContext *ctx)' : this function is aborted.");
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    if (ctx->addorsub_op().empty()) {
        return visit(ctx->term(0));
    }
    bool no_firstval = true;
    Single res{};
    Python3Parser::Addorsub_opContext *op;
    Single thisval;
    for (auto *child : ctx->children) {
        if (auto *term = dynamic_cast<Python3Parser::TermContext *>(child)) {
            if (no_firstval) {
                no_firstval = false;
                res = std::get<Single>(std::any_cast<Value>(visit(term)));
            } else {
                thisval = std::get<Single>(std::any_cast<Value>(visit(term)));
                if (op->ADD() != nullptr) {
                    res = res + thisval;
                } else if (op->MINUS() != nullptr) {
                    res = res - thisval;
                }
            }
        } else {
            op = dynamic_cast<Python3Parser::Addorsub_opContext *>(child);
        }
    }
    return Value{res};
}

// 弃用
std::any EvalVisitor::visitAddorsub_op(Python3Parser::Addorsub_opContext *ctx) {
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitAddorsub_op(Python3Parser::Addorsub_opContext *ctx)' : this function is aborted.");
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    if (ctx->muldivmod_op().empty()) {
        return visit(ctx->factor(0));
    }
    bool no_firstval = true;
    Single res{};
    Python3Parser::Muldivmod_opContext *op;
    Single thisval;
    for (auto *child : ctx->children) {
        if (auto *factor = dynamic_cast<Python3Parser::FactorContext *>(child)) {
            if (no_firstval) {
                no_firstval = false;
                res = std::get<Single>(std::any_cast<Value>(visit(factor)));
            } else {
                thisval = std::get<Single>(std::any_cast<Value>(visit(factor)));
                if (op->STAR() != nullptr) {
                    res = res * thisval;
                } else if (op->DIV() != nullptr) {
                    res = res / thisval;
                } else if (op->IDIV() != nullptr) {
                    res = res | thisval;
                } else if (op->MOD() != nullptr) {
                    res = res % thisval;
                }
            }
        } else {
            op = dynamic_cast<Python3Parser::Muldivmod_opContext *>(child);
        }
    }
    return Value{res};
}

// 弃用
std::any EvalVisitor::visitMuldivmod_op(Python3Parser::Muldivmod_opContext *ctx) {
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitMuldivmod_op(Python3Parser::Muldivmod_opContext *ctx)' : this function is aborted.");
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    // + factor
    if (ctx->ADD() != nullptr) {
        return Value{+std::get<Single>(std::any_cast<Value>(visitFactor(ctx->factor())))};
    }

    // - factor
    if (ctx->MINUS() != nullptr) {
        return Value{-std::get<Single>(std::any_cast<Value>(visitFactor(ctx->factor())))};
    }

    // atom_expr
    return visitAtom_expr(ctx->atom_expr());
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    if (ctx->trailer() == nullptr) {
        return visitAtom(ctx->atom());
    } // 不是函数调用
    if (ctx->atom()->getText() == "print") {
        if (ctx->trailer()->arglist() == nullptr) {
            putchar('\n');
        } else {
            visit(ctx->trailer()->arglist());
        }
        return Value{Single{None{}}};
    } // [内置函数] print
    if (ctx->atom()->getText() == "int") {
        return Value{
                 Single{
                    convertToInt(
                        std::get<Single>(
                            std::any_cast<Value>(
                                visit(ctx->trailer()->arglist()->argument(0))
                            )
                        )
                    )
                 }
               };
    } // [内置函数] int
    if (ctx->atom()->getText() == "float") {
        return Value{
                 Single{
                    convertToFloat(
                        std::get<Single>(
                            std::any_cast<Value>(
                                visit(ctx->trailer()->arglist()->argument(0))
                            )
                        )
                    )
                 }
               };
    } // [内置函数] float
    if (ctx->atom()->getText() == "str") {
        return Value{
                 Single{
                    convertToStr(
                        std::get<Single>(
                            std::any_cast<Value>(
                                visit(ctx->trailer()->arglist()->argument(0))
                            )
                        )
                    )
                 }
               };
    } // [内置函数] str
    if (ctx->atom()->getText() == "bool") {
        return Value{
                 Single{
                    convertToBool(
                        std::get<Single>(
                            std::any_cast<Value>(
                                visit(ctx->trailer()->arglist()->argument(0))
                            )
                        )
                    )
                 }
               };
    } // [内置函数] bool
    auto &call_func = functions_[ctx->atom()->getText()]; // 获取函数信息
    auto scope = std::get<0>(call_func); // 搭建初始作用域
    auto &name_set = std::get<1>(call_func);
    if (ctx->trailer()->arglist() != nullptr) {
        int name_pointer = 0;
        for (auto *child : ctx->trailer()->arglist()->children) {
            if (auto *argument = dynamic_cast<Python3Parser::ArgumentContext *>(child)) {
                if (argument->ASSIGN() == nullptr) { // 按位置赋值
                    scope[name_set[name_pointer]] = std::get<Single>(std::any_cast<Value>(visit(argument->test(0))));
                    ++name_pointer;
                } else { // 按名称赋值
                    scope[argument->test(0)->getText()] = std::get<Single>(std::any_cast<Value>(visit(argument->test(1))));
                }
            }
        }
    } // 处理传参
    // 传参完毕，local作用域建立完成
    locals_.push(scope);
    Result temp_result = std::any_cast<Result>(visit(std::get<2>(call_func))); // 访问suite，获得执行结果
    locals_.pop();
    return temp_result.value_;
}

// 弃用
std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    throw std::runtime_error("[Error] In function 'EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx)' : this function is aborted.");
}

// 此函数只能用于获得值，不负责获取变量名
std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    // 因为这个atom可能出现在LHS或者RHS，所以不能一次性实现两种模式
    // 因此，统一规定，它应该读取值，不负责回传变量名
    if (ctx->NAME() != nullptr) {
        return Value{readVariable(ctx->NAME()->getText())};
    }

    // Int 或者 Float
    if (ctx->NUMBER() != nullptr) {
        std::string number_text = ctx->NUMBER()->getText();
        for (auto i : number_text) {
            if (i == '.') {
                // Float类型
                return Value{Single{convertToFloat(number_text)}};
            }
        }
        // Int类型
        return Value{Single{convertToInt(number_text)}};
    }

    // None
    if (ctx->NONE() != nullptr) {
        return Value{Single{None{}}};
    }

    // True
    if (ctx->TRUE() != nullptr) {
        return Value{Single{Bool{true}}};
    }

    // False
    if (ctx->FALSE() != nullptr) {
        return Value{Single{Bool{false}}};
    }

    // Str
    if (ctx->STRING().size()) {
        Str res{};
        for (auto small_string : ctx->STRING()) {
            std::string temp_str = small_string->getText();
            for (int i = 1; i + 1 < temp_str.size(); ++i) {
                // 处理转义
                if (temp_str[i] == '\\') {
                    // 下面可以放心 + 1，因为保证输入合法
                    ++i; // 跳到下一个字符
                    long long value = 0;
                    switch (temp_str[i]) {
                        case 'n': res.push_back('\n'); break;
                        case 'r': res.push_back('\r'); break;
                        case 't': res.push_back('\t'); break;
                        case 'v': res.push_back('\v'); break;
                        case 'f': res.push_back('\f'); break;
                        case '\\': res.push_back('\\'); break;
                        case '\'': res.push_back('\''); break;
                        case '\"': res.push_back('\"'); break;
                        case '\?': res.push_back('\?'); break;
                        case 'x': // 十六进制转义
                            ++i; // 这就指向了第一个数码
                            value = std::isdigit(temp_str[i])  
                                    ? (temp_str[i] - '0')  
                                    : (temp_str[i] - (temp_str[i] >= 'a' ? 'a' : 'A') + 10);
                            ++i; // 这就指向了第二个数码
                            value = (value << 4) 
                                    + (std::isdigit(temp_str[i]) 
                                      ? (temp_str[i] - '0') 
                                      : (temp_str[i] - (temp_str[i] >= 'a' ? 'a' : 'A') + 10));
                            res.push_back(static_cast<char>(value));
                            break;
                        default: // 八进制转义
                            value = (temp_str[i] - '0');
                            ++i; // 这就指向了第二个数码       
                            value = (value << 3) + (temp_str[i] - '0');
                            ++i; // 这就指向了第三个数码       
                            value = (value << 3) + (temp_str[i] - '0');
                            res.push_back(static_cast<char>(value));
                            break;
                    }
                } else {
                    res.push_back(temp_str[i]);
                }
            }
        }
        return Value{Single{res}};
    }

    // (test) 直接忽略掉OPEN_PAREN（左括号）和CLOSE_PAREN（右括号）
    if (ctx->test() != nullptr) {
        return visit(ctx->test());
    }

    // format string
    if (ctx->format_string() != nullptr) {
        return visit(ctx->format_string());
    }

    throw std::runtime_error("[Error] In function 'EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx)' : invalid context.");
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    Str res{};
    for (auto *child : ctx->children) {
        if (auto *testlist = dynamic_cast<Python3Parser::TestlistContext *>(child)) { // 如果不是nullptr，就是testlist
            res += convertToStr(std::get<Single>(std::any_cast<Value>(visitTestlist(testlist))));
        } else if (auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
            auto terminal_type = terminal->getSymbol()->getType();
            if (terminal_type == Python3Parser::FORMAT_STRING_LITERAL) {
                std::string temp_str = terminal->getText();
                for (int i = 0; i < temp_str.size(); ++i) {
                    // 处理反斜杠转义
                    if (temp_str[i] == '\\') {
                        // 下面可以放心 + 1，因为保证输入合法
                        ++i; // 跳到下一个字符
                        long long value = 0;
                        switch (temp_str[i]) {
                            case 'n': res.push_back('\n'); break;
                            case 'r': res.push_back('\r'); break;
                            case 't': res.push_back('\t'); break;
                            case 'v': res.push_back('\v'); break;
                            case 'f': res.push_back('\f'); break;
                            case '\\': res.push_back('\\'); break;
                            case '\'': res.push_back('\''); break;
                            case '\"': res.push_back('\"'); break;
                            case '\?': res.push_back('\?'); break;
                            case 'x': // 十六进制转义
                                ++i; // 这就指向了第一个数码
                                value = std::isdigit(temp_str[i])  
                                        ? (temp_str[i] - '0')  
                                        : (temp_str[i] - (temp_str[i] >= 'a' ? 'a' : 'A') + 10);
                                ++i; // 这就指向了第二个数码
                                value = (value << 4) 
                                        + (std::isdigit(temp_str[i]) 
                                        ? (temp_str[i] - '0') 
                                        : (temp_str[i] - (temp_str[i] >= 'a' ? 'a' : 'A') + 10));
                                res.push_back(static_cast<char>(value));
                                break;
                            default: // 八进制转义
                                value = (temp_str[i] - '0');
                                ++i; // 这就指向了第二个数码       
                                value = (value << 3) + (temp_str[i] - '0');
                                ++i; // 这就指向了第三个数码       
                                    value = (value << 3) + (temp_str[i] - '0');
                                    res.push_back(static_cast<char>(value));
                                    break;
                        }
                    } else if (temp_str[i] == '{' || temp_str[i] == '}') { // 如果在这里看见大括号，必然是转义大括号
                        res.push_back(temp_str[++i]);
                    } else {
                        res.push_back(temp_str[i]);
                    }
                }
            }
        }
    }
    return Value{Single{res}};
}

// 此函数只能用于获得值，不负责获取变量名
std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    if (ctx->test().empty()) {
        throw std::runtime_error("[Error] In function 'EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx)' : empty testlist.");
    }
    if (ctx->COMMA().empty()) {
        return visit(ctx->test(0)); // visitTest也是读值专用函数，返回Value
    }
    Tuple temp{};
    for (auto *child : ctx->children) {
        if (auto *test = dynamic_cast<Python3Parser::TestContext *>(child)) {
            temp.push_back(std::get<Single>(std::any_cast<Value>(visit(test))));
        }
    }
    return Value{temp};
}

// 此函数为print专用函数
std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    std::vector<Single> output_value{};
    for (auto *child : ctx->children) {
        if (auto *argument = dynamic_cast<Python3Parser::ArgumentContext *>(child)) {
            // 注意，必须先求出所有的输出参数，再输出，否则万一求参的时候也有输出，直接就炸了
            output_value.push_back(std::get<Single>(std::any_cast<Value>(visit(argument))));
        }
    }
    int cnt = 0;
    for (auto value : output_value) {
        std::cout << value;
        if ((++cnt) < output_value.size()) {
            std::putchar(' ');
        }
    }
    std::putchar('\n');
    return Value{Single{None{}}};
}

// 此函数只准用于读取第一个test的字面值或者变量值，总之一定是获取值
std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    if (ctx->ASSIGN() != nullptr) {
        throw std::runtime_error("[Error] In function 'EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx)' : invalid visit.");
    }
    return visit(ctx->test(0)); // 只准用于读取第一个test的字面值或者变量值
}