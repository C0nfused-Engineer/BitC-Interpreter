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

// Step 18: parent-chained scope. set() writes to THIS scope only (declarations).
// assign() walks up to find an existing binding and updates it there.
// get() / has() walk up the chain.
class Environment {
public:
    explicit Environment(Environment* parent = nullptr) : parent(parent) {}

    void     set(const std::string& name, BitValue val);
    void     assign(const std::string& name, BitValue val);
    BitValue get(const std::string& name);
    bool     has(const std::string& name);

private:
    std::unordered_map<std::string, BitValue> vars;
    Environment* parent;
};

class Interpreter {
public:
    void runProgram(const std::vector<NodePtr>& statements);

private:
    // Step 19: top-level environment and function table
    Environment  globalEnv;
    Environment* currentEnv = &globalEnv;
    std::unordered_map<std::string, FuncDeclNode*> functions;

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
    BitValue evalFuncCall(FuncCallNode* node);  // Step 20
    BitValue evalIn();                          // Step 21
};