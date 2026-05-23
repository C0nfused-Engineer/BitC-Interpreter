#include "interpreter.h"
#include <stdexcept>
#include <iostream>

// ─── Environment ─────────────────────────────────────────────────────────────

// Declare in the current scope (variable / parameter declaration).
void Environment::set(const std::string& name, BitValue val) {
    vars[name] = val;
}

// Update an existing binding — walks up the chain to find where it lives.
// Used by assignment (x = expr) so that a variable declared in an outer
// scope is updated there, not shadowed in the current scope.
void Environment::assign(const std::string& name, BitValue val) {
    if (vars.count(name)) {
        vars[name] = val;
        return;
    }
    if (parent) {
        parent->assign(name, val);
        return;
    }
    throw std::runtime_error("Assignment to undeclared variable '" + name + "'");
}

BitValue Environment::get(const std::string& name) {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second;
    if (parent)           return parent->get(name);
    throw std::runtime_error("Undefined variable '" + name + "'");
}

bool Environment::has(const std::string& name) {
    if (vars.count(name)) return true;
    if (parent)           return parent->has(name);
    return false;
}

// ─── Top level ───────────────────────────────────────────────────────────────

// Step 19: two-pass execution.
// Pass 1 — register every function declaration in the function table so
//           forward calls resolve before any code runs.
// Pass 2 — execute all non-function statements in order.
void Interpreter::runProgram(const std::vector<NodePtr>& statements) {
    // Pass 1: collect function declarations
    for (auto& stmt : statements) {
        if (auto* fn = dynamic_cast<FuncDeclNode*>(stmt.get())) {
            if (functions.count(fn->name))
                throw std::runtime_error("Duplicate function definition '"
                    + fn->name + "'");
            functions[fn->name] = fn;
        }
    }

    // Pass 2: execute top-level statements (skip function declarations)
    for (auto& stmt : statements) {
        if (!dynamic_cast<FuncDeclNode*>(stmt.get())) {
            execStatement(stmt.get());
        }
    }
}

// ─── Statement dispatch ───────────────────────────────────────────────────────

void Interpreter::execStatement(ASTNode* node) {
    if (auto* n = dynamic_cast<VarDeclNode*>(node)) { execVarDecl(n);  return; }
    if (auto* n = dynamic_cast<ArrayDeclNode*>(node)) { execArrayDecl(n); return; }
    if (auto* n = dynamic_cast<AssignNode*>(node)) { execAssign(n);   return; }
    if (auto* n = dynamic_cast<IfNode*>(node)) { execIf(n);       return; }
    if (auto* n = dynamic_cast<WhileNode*>(node)) { execWhile(n);    return; }
    if (auto* n = dynamic_cast<PrintNode*>(node)) { execPrint(n);    return; }

    if (dynamic_cast<BreakNode*>(node)) { throw BreakSignal{}; }
    if (dynamic_cast<ContinueNode*>(node)) { throw ContinueSignal{}; }

    if (auto* n = dynamic_cast<ReturnNode*>(node)) {
        ReturnSignal sig;
        if (n->value) sig.value = evalExpr(n->value.get());
        throw sig;
    }

    // Step 20: function call used as a statement (void call or discarded value)
    if (auto* n = dynamic_cast<FuncCallNode*>(node)) {
        evalFuncCall(n);
        return;
    }

    // FuncDeclNode is handled in runProgram pass 1 and never reaches here
    // at the top level. Inside a function body it would be an error.
    if (dynamic_cast<FuncDeclNode*>(node)) {
        throw std::runtime_error("Nested function definitions are not allowed");
    }

    throw std::runtime_error("Unknown statement node");
}

// ─── Statements ──────────────────────────────────────────────────────────────

void Interpreter::execVarDecl(VarDeclNode* node) {
    BitValue val = evalExpr(node->initializer.get());
    if (val.isArray)
        throw std::runtime_error("Cannot assign array to plain bit variable '"
            + node->name + "'");
    currentEnv->set(node->name, val);
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
    currentEnv->set(node->name, val);
}

// Step 24: assignment uses assign() which walks the scope chain.
void Interpreter::execAssign(AssignNode* node) {
    BitValue val = evalExpr(node->value.get());
    currentEnv->assign(node->name, val);
}

// Step 24: if/while bodies each get a child scope.
void Interpreter::execIf(IfNode* node) {
    BitValue cond = evalExpr(node->condition.get());
    if (cond.isArray)
        throw std::runtime_error("If condition must be a single bit");

    Environment childEnv(currentEnv);
    Environment* saved = currentEnv;
    currentEnv = &childEnv;

    try {
        if (cond.bit) {
            for (auto& stmt : node->thenBody)
                execStatement(stmt.get());
        }
        else {
            for (auto& stmt : node->elseBody)
                execStatement(stmt.get());
        }
    }
    catch (...) {
        currentEnv = saved;
        throw;
    }

    currentEnv = saved;
}

void Interpreter::execWhile(WhileNode* node) {
    while (true) {
        BitValue cond = evalExpr(node->condition.get());
        if (cond.isArray)
            throw std::runtime_error("While condition must be a single bit");
        if (!cond.bit) break;

        Environment childEnv(currentEnv);
        Environment* saved = currentEnv;
        currentEnv = &childEnv;

        try {
            for (auto& stmt : node->body)
                execStatement(stmt.get());
        }
        catch (BreakSignal&) { currentEnv = saved; break; }
        catch (ContinueSignal&) { currentEnv = saved; continue; }
        catch (...) {
            currentEnv = saved;
            throw;
        }

        currentEnv = saved;
    }
}

// Step 23: scalar prints bare 0/1; array prints [1, 0, 1, 0] with ", " separator.
void Interpreter::execPrint(PrintNode* node) {
    BitValue val = evalExpr(node->value.get());
    if (val.isArray) {
        std::cout << "[";
        for (int i = 0; i < (int)val.bits.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << val.bits[i];
        }
        std::cout << "]\n";
    }
    else {
        std::cout << val.bit << "\n";
    }
}

// ─── Expression evaluator ────────────────────────────────────────────────────

BitValue Interpreter::evalExpr(ASTNode* node) {
    if (auto* n = dynamic_cast<BitLiteralNode*>(node))   return { false, n->value, {} };
    if (auto* n = dynamic_cast<IdentifierNode*>(node))   return evalIdentifier(n);
    if (auto* n = dynamic_cast<BinaryOpNode*>(node))     return evalBinaryOp(n);
    if (auto* n = dynamic_cast<UnaryOpNode*>(node))      return evalUnaryOp(n);
    if (auto* n = dynamic_cast<FuncCallNode*>(node))     return evalFuncCall(n);
    if (dynamic_cast<InNode*>(node))                     return evalIn();

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
    return currentEnv->get(node->name);
}

// Step 20: function call evaluation.
BitValue Interpreter::evalFuncCall(FuncCallNode* node) {
    // Look up the function
    auto it = functions.find(node->name);
    if (it == functions.end())
        throw std::runtime_error("Undefined function '" + node->name + "'");

    FuncDeclNode* fn = it->second;

    // Check argument count
    if (node->args.size() != fn->params.size())
        throw std::runtime_error("Function '" + node->name + "' expects "
            + std::to_string(fn->params.size()) + " argument(s), got "
            + std::to_string(node->args.size()));

    // Evaluate arguments in the CALLER's environment
    std::vector<BitValue> argVals;
    for (auto& arg : node->args)
        argVals.push_back(evalExpr(arg.get()));

    // Type-check arguments against parameters
    for (int i = 0; i < (int)fn->params.size(); i++) {
        const ParamNode& param = fn->params[i];
        const BitValue& val = argVals[i];

        if (param.isArray && !val.isArray)
            throw std::runtime_error("Argument " + std::to_string(i + 1)
                + " of '" + node->name + "' must be a bit array");
        if (!param.isArray && val.isArray)
            throw std::runtime_error("Argument " + std::to_string(i + 1)
                + " of '" + node->name + "' must be a scalar bit");
        if (param.isArray && (int)val.bits.size() != param.arraySize)
            throw std::runtime_error("Argument " + std::to_string(i + 1)
                + " of '" + node->name + "' expects bit["
                + std::to_string(param.arraySize) + "], got bit["
                + std::to_string(val.bits.size()) + "]");
    }

    // Create a fresh environment for the function body.
    // Parent is globalEnv so functions can read top-level variables
    // but cannot see the caller's locals.
    Environment callEnv(&globalEnv);
    Environment* saved = currentEnv;
    currentEnv = &callEnv;

    // Bind parameters
    for (int i = 0; i < (int)fn->params.size(); i++)
        currentEnv->set(fn->params[i].name, argVals[i]);

    // Execute body, catching ReturnSignal
    BitValue returnVal;
    bool     didReturn = false;

    try {
        for (auto& stmt : fn->body)
            execStatement(stmt.get());
    }
    catch (ReturnSignal& sig) {
        returnVal = sig.value;
        didReturn = true;
    }
    catch (...) {
        currentEnv = saved;
        throw;
    }

    currentEnv = saved;

    // Enforce return requirements
    if (!fn->returnsVoid && !didReturn)
        throw std::runtime_error("Function '" + node->name
            + "' must return a value");

    return returnVal;
}

// Step 21: in() — reads from stdin, loops until a valid bit literal is entered.
BitValue Interpreter::evalIn() {
    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {
            throw std::runtime_error("Unexpected end of input in in()");
        }

        // trim whitespace
        int start = 0, end = (int)line.size() - 1;
        while (start <= end && isspace((unsigned char)line[start])) start++;
        while (end >= start && isspace((unsigned char)line[end]))   end--;
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed == "0" || trimmed == "false") return { false, 0, {} };
        if (trimmed == "1" || trimmed == "true")  return { false, 1, {} };

        std::cerr << "Input must be 0, 1, true, or false. Try again.\n";
    }
}

BitValue Interpreter::evalBinaryOp(BinaryOpNode* node) {
    BitValue left = evalExpr(node->left.get());
    BitValue right = evalExpr(node->right.get());

    // Array operations — both sides must be arrays of equal length
    if (left.isArray && right.isArray) {
        if (left.bits.size() != right.bits.size())
            throw std::runtime_error("Array length mismatch in '"
                + node->op + "': left is "
                + std::to_string(left.bits.size()) + " bits, right is "
                + std::to_string(right.bits.size()) + " bits");

        int n = (int)left.bits.size();
        BitValue result;
        result.isArray = true;

        if (node->op == "&") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] & right.bits[i]);
            return result;
        }
        if (node->op == "|") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] | right.bits[i]);
            return result;
        }
        if (node->op == "^") {
            for (int i = 0; i < n; i++)
                result.bits.push_back(left.bits[i] ^ right.bits[i]);
            return result;
        }
        if (node->op == "==") return { false, left.bits == right.bits ? 1 : 0, {} };
        if (node->op == "!=") return { false, left.bits != right.bits ? 1 : 0, {} };

        throw std::runtime_error("Operator '" + node->op
            + "' is not supported for arrays");
    }

    // Type mismatch — one array, one scalar
    if (left.isArray != right.isArray)
        throw std::runtime_error("Type mismatch: cannot mix bit and bit array with '"
            + node->op + "'");

    // Scalar operations
    int l = left.bit, r = right.bit;
    if (node->op == "&")  return { false, l & r,    {} };
    if (node->op == "|")  return { false, l | r,    {} };
    if (node->op == "^")  return { false, l ^ r,    {} };
    if (node->op == "<<") return { false, (l << r) & 1, {} };
    if (node->op == ">>") return { false, (l >> r) & 1, {} };
    if (node->op == "==") return { false, l == r,   {} };
    if (node->op == "!=") return { false, l != r,   {} };
    if (node->op == "<")  return { false, l < r,    {} };
    if (node->op == ">")  return { false, l > r,    {} };

    throw std::runtime_error("Unsupported operator '" + node->op + "'");
}

BitValue Interpreter::evalUnaryOp(UnaryOpNode* node) {
    BitValue val = evalExpr(node->operand.get());

    if (val.isArray) {
        BitValue result;
        result.isArray = true;
        if (node->op == "~" || node->op == "!") {
            for (int b : val.bits)
                result.bits.push_back(b ^ 1);
            return result;
        }
        throw std::runtime_error("Unsupported unary operator '"
            + node->op + "' for arrays");
    }

    if (node->op == "~") return { false, val.bit ^ 1, {} };
    if (node->op == "!") return { false, !val.bit,    {} };

    throw std::runtime_error("Unsupported unary operator '" + node->op + "'");
}