#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "ast.h"

struct BitValue {
    bool isArray = false;
    int bit = 0;
    std::vector<int> bits;
};

// signals for break/continue/return
struct BreakSignal {};
struct ContinueSignal {};
struct ReturnSignal { BitValue value; };

class Environment {
public:
    void     set(const std::string& name, BitValue val);
    BitValue get(const std::string& name);
    bool     has(const std::string& name);

private:
    std::unordered_map<std::string, BitValue> vars;
};

class Interpreter {
public:
    void runProgram(const std::vector<NodePtr>& statements);

private:
    Environment env;

    void     execStatement(ASTNode* node);
    BitValue evalExpr(ASTNode* node);

    void     execVarDecl(VarDeclNode* node);
    void     execArrayDecl(ArrayDeclNode* node);
    void     execAssign(AssignNode* node);
    void     execIf(IfNode* node);
    void     execWhile(WhileNode* node);
    void     execPrint(PrintNode* node);

    BitValue evalBinaryOp(BinaryOpNode* node);
    BitValue evalUnaryOp(UnaryOpNode* node);
    BitValue evalIdentifier(IdentifierNode* node);
};