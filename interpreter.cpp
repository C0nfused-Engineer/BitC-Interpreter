#include "interpreter.h"
#include <stdexcept>
#include <iostream>

// ??? Environment ?????????????????????????????????????????????????????????????

void Environment::set(const std::string& name, BitValue val) {
    vars[name] = val;
}

BitValue Environment::get(const std::string& name) {
    auto it = vars.find(name);
    if (it == vars.end())
        throw std::runtime_error("Undefined variable '" + name + "'");
    return it->second;
}

bool Environment::has(const std::string& name) {
    return vars.count(name) > 0;
}

// ??? top level ???????????????????????????????????????????????????????????????

void Interpreter::runProgram(const std::vector<NodePtr>& statements) {
    for (auto& stmt : statements) {
        execStatement(stmt.get());
    }
}

// ??? statement dispatch ???????????????????????????????????????????????????????

void Interpreter::execStatement(ASTNode* node) {
    if (auto* n = dynamic_cast<VarDeclNode*>(node)) {
        execVarDecl(n); return;
    }
    if (auto* n = dynamic_cast<ArrayDeclNode*>(node)) {
        execArrayDecl(n); return;
    }
    if (auto* n = dynamic_cast<AssignNode*>(node)) {
        execAssign(n); return;
    }
    if (auto* n = dynamic_cast<IfNode*>(node)) {
        execIf(n); return;
    }
    if (auto* n = dynamic_cast<WhileNode*>(node)) {
        execWhile(n); return;
    }
    if (auto* n = dynamic_cast<PrintNode*>(node)) {
        execPrint(n); return;
    }
    if (dynamic_cast<BreakNode*>(node)) {
        throw BreakSignal{};
    }
    if (dynamic_cast<ContinueNode*>(node)) {
        throw ContinueSignal{};
    }
    if (auto* n = dynamic_cast<ReturnNode*>(node)) {
        ReturnSignal sig;
        if (n->value) sig.value = evalExpr(n->value.get());
        throw sig;
    }

    throw std::runtime_error("Unknown statement node");
}

// ??? statements ??????????????????????????????????????????????????????????????

void Interpreter::execVarDecl(VarDeclNode* node) {
    BitValue val = evalExpr(node->initializer.get());
    if (val.isArray)
        throw std::runtime_error("Cannot assign array to plain bit variable '"
            + node->name + "'");
    env.set(node->name, val);
}

void Interpreter::execArrayDecl(ArrayDeclNode* node) {
    BitValue val = evalExpr(node->initializer.get());
    if (!val.isArray)
        throw std::runtime_error("Expected array initializer for '"
            + node->name + "'");
    if ((int)val.bits.size() != node->size)
        throw std::runtime_error("Array size mismatch for '" + node->name
            + "': declared " + std::to_string(node->size)
            + " but got " + std::to_string(val.bits.size()));
    env.set(node->name, val);
}

void Interpreter::execAssign(AssignNode* node) {
    if (!env.has(node->name))
        throw std::runtime_error("Assignment to undeclared variable '"
            + node->name + "'");
    BitValue val = evalExpr(node->value.get());
    env.set(node->name, val);
}

void Interpreter::execIf(IfNode* node) {
    BitValue cond = evalExpr(node->condition.get());
    if (cond.isArray)
        throw std::runtime_error("If condition must be a single bit");

    if (cond.bit) {
        for (auto& stmt : node->thenBody)
            execStatement(stmt.get());
    }
    else {
        for (auto& stmt : node->elseBody)
            execStatement(stmt.get());
    }
}

void Interpreter::execWhile(WhileNode* node) {
    while (true) {
        BitValue cond = evalExpr(node->condition.get());
        if (cond.isArray)
            throw std::runtime_error("While condition must be a single bit");
        if (!cond.bit) break;

        try {
            for (auto& stmt : node->body)
                execStatement(stmt.get());
        }
        catch (BreakSignal&) { break; }
        catch (ContinueSignal&) { continue; }
    }
}

void Interpreter::execPrint(PrintNode* node) {
    BitValue val = evalExpr(node->value.get());
    if (val.isArray) {
        std::cout << "[";
        for (int i = 0; i < (int)val.bits.size(); i++) {
            std::cout << val.bits[i];
            if (i < (int)val.bits.size() - 1) std::cout << ",";
        }
        std::cout << "]\n";
    }
    else {
        std::cout << val.bit << "\n";
    }
}

// ??? expression evaluator ????????????????????????????????????????????????????

BitValue Interpreter::evalExpr(ASTNode* node) {
    if (auto* n = dynamic_cast<BitLiteralNode*>(node)) {
        return { false, n->value, {} };
    }
    if (auto* n = dynamic_cast<IdentifierNode*>(node)) {
        return evalIdentifier(n);
    }
    if (auto* n = dynamic_cast<BinaryOpNode*>(node)) {
        return evalBinaryOp(n);
    }
    if (auto* n = dynamic_cast<UnaryOpNode*>(node)) {
        return evalUnaryOp(n);
    }
    if (auto* n = dynamic_cast<ArrayLiteralNode*>(node)) {
        BitValue result;
        result.isArray = true;
        for (auto& elem : n->elements) {
            BitValue v = evalExpr(elem.get());
            if (v.isArray)
                throw std::runtime_error("Nested arrays are not supported");
            result.bits.push_back(v.bit);
        }
        return result;
    }

    throw std::runtime_error("Unknown expression node");
}

BitValue Interpreter::evalIdentifier(IdentifierNode* node) {
    return env.get(node->name);
}

BitValue Interpreter::evalBinaryOp(BinaryOpNode* node) {
    BitValue left = evalExpr(node->left.get());
    BitValue right = evalExpr(node->right.get());

    // array operations — both sides must be arrays of equal size
    if (left.isArray && right.isArray) {
        if (left.bits.size() != right.bits.size())
            throw std::runtime_error("Array size mismatch in binary operation");

        BitValue result;
        result.isArray = true;
        int n = left.bits.size();

        if (node->op == "&") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] & right.bits[i]);
        }
        else if (node->op == "|") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] | right.bits[i]);
        }
        else if (node->op == "^") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] ^ right.bits[i]);
        }
        else if (node->op == "<<") {
            // left shift — shift bits left, fill with 0
            result.bits = std::vector<int>(n, 0);
            int shift = 1; // for now shift by 1
            for (int i = 0; i < n - shift; i++)
                result.bits[i] = left.bits[i + shift];
        }
        else if (node->op == ">>") {
            result.bits = std::vector<int>(n, 0);
            int shift = 1;
            for (int i = shift; i < n; i++)
                result.bits[i] = left.bits[i - shift];
        }
        else if (node->op == "==") {
            return { false, left.bits == right.bits ? 1 : 0, {} };
        }
        else if (node->op == "!=") {
            return { false, left.bits != right.bits ? 1 : 0, {} };
        }
        else {
            throw std::runtime_error("Unsupported operator '" + node->op
                + "' for arrays");
        }
        return result;
    }

    // scalar operations — both sides must be single bits
    if (!left.isArray && !right.isArray) {
        int l = left.bit, r = right.bit;
        if (node->op == "&")  return { false, l & r,  {} };
        if (node->op == "|")  return { false, l | r,  {} };
        if (node->op == "^")  return { false, l ^ r,  {} };
        if (node->op == "&&") return { false, l && r, {} };
        if (node->op == "||") return { false, l || r, {} };
        if (node->op == "==") return { false, l == r, {} };
        if (node->op == "!=") return { false, l != r, {} };
        if (node->op == "<")  return { false, l < r, {} };
        if (node->op == ">")  return { false, l > r, {} };

        throw std::runtime_error("Unsupported operator '" + node->op + "'");
    }

    throw std::runtime_error("Cannot mix scalar and array in binary operation");
}

BitValue Interpreter::evalUnaryOp(UnaryOpNode* node) {
    BitValue val = evalExpr(node->operand.get());

    if (val.isArray) {
        BitValue result;
        result.isArray = true;
        if (node->op == "~") {
            for (int b : val.bits)
                result.bits.push_back(b ^ 1);
        }
        else {
            throw std::runtime_error("Unsupported unary operator '"
                + node->op + "' for arrays");
        }
        return result;
    }

    if (node->op == "~") return { false, val.bit ^ 1, {} };
    if (node->op == "!") return { false, !val.bit,    {} };

    throw std::runtime_error("Unsupported unary operator '" + node->op + "'");
}