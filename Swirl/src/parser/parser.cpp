#include <cmath>
#include <deque>
#include <iostream>
#include <memory>
#include <string>

#include <parser/parser.h>
#include <tokens/Tokens.h>
#include <unordered_map>

using namespace std::string_literals;

// Flags
short ang_ind = 0;
uint8_t is_first_t = 1;
uint8_t rd_param = 0;
uint8_t rd_func = 0;
uint8_t rd_param_cnt = 0;

std::vector<Node> Module{};
extern std::unordered_map<std::string, const char *> type_registry;
extern std::unordered_map<std::string, uint8_t> non_assign_binary_ops;

std::unordered_map<std::string, uint8_t> default_operator_precedence = {
        {"+",  0},
        {"-",  1},
        {"*",  2},
        {"/",  3},
        {"**", 4},
        {">>", 5},
        {"<<", 6},
        {"^",  7},
        {"|",  8},
        {"&",  9},
        {"~",  10}
};

Parser::Parser(TokenStream &tks) : m_Stream(tks) {}

Parser::~Parser() = default;

void Parser::next(bool swsFlg, bool snsFlg) {
    cur_rd_tok = m_Stream.next(swsFlg, snsFlg);
}

void appendModule(const Node &nd) {
//    Module.emplace_back(nd);
}

void traverse(std::shared_ptr<Expression> &expr) {
    for (auto elem: expr->evaluation_ord) {
        if (elem->getType() == ND_EXPR)
            traverse(elem);
        std::cout << elem->getType() << std::endl;
    }
}

void Parser::dispatch() {
    int br_ind = 0;
    int prn_ind = 0;
    const char *tmp_ident = "";
    const char *tmp_type = "";

    m_Stream.next();
    while (m_Stream.p_CurTk.type != NONE) {
        TokenType t_type = m_Stream.p_CurTk.type;
        std::string t_val = m_Stream.p_CurTk.value;

        if (t_type == KEYWORD) {
            if (t_val == "var" || t_val == "const") {
                parseVar();
            }
        }

        if (t_type == IDENT) {
            if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                appendModule(*parseCall());
            }
        }
        m_Stream.next();
    }
}

void Parser::parseVar() {
    Var var_node;
    var_node.is_const = m_Stream.p_CurTk.value == "const";
    var_node.var_ident = m_Stream.next().value;

    if (m_Stream.peek().type == PUNC && m_Stream.peek().value == ":") {
        next();
        var_node.var_type = m_Stream.next().value;
    }
    
    if (m_Stream.peek().type == OP && m_Stream.peek().value == "=") {
        var_node.initialized = true;
        next();
        next();
        parseExpr();
    }
}

std::unique_ptr<FuncCall> Parser::parseCall() {
    std::deque<Token> expr_tok_track{};
    std::unique_ptr<FuncCall> call_node = std::make_unique<FuncCall>();
    call_node->ident = m_Stream.p_CurTk.value;
    next();
    next();

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
        return call_node;

    parseExpr( call_node->ident);
//    std::function<void()> parseArgs = [this, &call_node, &parseArgs]() -> void {
//        if (m_Stream.p_CurTk.value == "," && m_Stream.p_CurTk.type == PUNC) {
//            m_Stream.next();
//            parseExpr(&call_node->args, call_node->ident);
//        }
//        parseArgs();
//    };

    while (true) {
        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
            break;
        if (m_Stream.p_CurTk.value == "," && m_Stream.p_CurTk.type == PUNC) {
            m_Stream.next();
            parseExpr(call_node->ident);
        }
    }

//    parseArgs();
    m_Stream.next();
    return call_node;
}

void Parser::parseExpr(const std::string id) {
    std::vector<std::shared_ptr<Expression>> expr{};
    std::vector<std::shared_ptr<Expression>> current_expr_grp_ptr{};

    unsigned int paren_cnt = 0;

    auto push_to_expr = [&expr, &current_expr_grp_ptr](const std::shared_ptr<Expression>& node) -> void {
        if (current_expr_grp_ptr.empty()) { expr.emplace_back(node); }
        else { current_expr_grp_ptr.back()->evaluation_ord.emplace_back(node); }
    };

    std::cout << "parsing expression..." << std::endl;
    while (!m_Stream.eof()) {
        if (m_Stream.p_CurTk.type == IDENT && m_Stream.peek().type == PUNC && m_Stream.peek().value == "(")
            push_to_expr(parseCall());

        if (m_Stream.p_CurTk.type == STRING) {
            push_to_expr(std::make_shared<String>(m_Stream.p_CurTk.value));
            m_Stream.next();
            continue;
        }

        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "(") {
            paren_cnt++;
            auto expr_grp = std::make_shared<Expression>();
            push_to_expr(expr_grp);
            current_expr_grp_ptr.emplace_back(expr_grp);
            m_Stream.next();
            continue;
        }

        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")") {
            paren_cnt--;
            current_expr_grp_ptr.pop_back();
            m_Stream.next();
            continue;
        }

        if (m_Stream.p_CurTk.type == NUMBER) {
            if (m_Stream.p_CurTk.value.find('.') != std::string::npos)
                push_to_expr(std::make_shared<Double>(std::stod(m_Stream.p_CurTk.value)));
            else
                push_to_expr(std::make_shared<Int>(std::stoi(m_Stream.p_CurTk.value)));
            m_Stream.next();
            continue;
        }

        if (m_Stream.p_CurTk.type == IDENT) {
            push_to_expr(std::make_shared<Ident>(m_Stream.p_CurTk.value));
            m_Stream.next();
            continue;
        }

        if (!non_assign_binary_ops.contains(m_Stream.p_CurTk.value))
            break;
        else {
            push_to_expr(std::make_shared<Op>(m_Stream.p_CurTk.value));
            m_Stream.next();
            continue;
        }
    }

//    for (auto& elem  : expr) {
//        if (elem->getType() == ND_EXPR) {
//            for (auto f: elem->evaluation_ord) {
//                std::cout << "Yes! " << std::endl;
//                if (elem->getType() == ND_EXPR) {
//                    for (auto f: elem->evaluation_ord) {
//                        std::cout << "Yes! " << std::endl;
//                    }
//                }
//            }
//        } else std::cout << "E\n";
//    }
}
