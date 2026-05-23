#pragma once
#include <string>
#include <vector>
#include <memory>

struct ASTNode {
	virtual ~ASTNode() = default;
	int line = 0;
	int col = 0;
};

using NodePtr = std::unique_ptr<ASTNode>;

struct BitLiteralNode : ASTNode {
	int value; // 0 or 1
};

struct NumberNode : ASTNode {
	int value;
};

struct IdentifierNode : ASTNode {
	std::string name;
};

struct BinaryOpNode : ASTNode {
	std::string op;
	NodePtr left;
	NodePtr right;
};

struct UnaryOpNode : ASTNode {
	std::string op;
	NodePtr operand;
};

struct ArrayLiteralNode : ASTNode {
	std::vector<NodePtr> elements;
};

struct VarDeclNode : ASTNode {
	std::string name;
	NodePtr initializer;
};

struct ArrayDeclNode : ASTNode {
	std::string name;
	int size;
	NodePtr initializer;
};

struct AssignNode : ASTNode {
	std::string name;
	NodePtr value;
};

struct IfNode : ASTNode {
	NodePtr condition;
	std::vector<NodePtr> thenBody;
	std::vector<NodePtr> elseBody;
};

struct WhileNode : ASTNode {
	NodePtr condition;
	std::vector<NodePtr> body;
};

struct ReturnNode : ASTNode {
	NodePtr value;
};

struct BreakNode : ASTNode {};

struct ContinueNode : ASTNode {};

struct PrintNode : ASTNode {
	NodePtr value;
};

struct BlockNode : ASTNode {
	std::vector<NodePtr> statements;
};

struct ParamNode {
	bool        isArray = false;
	int         arraySize = 0; 
	std::string name;
};

struct FuncDeclNode : ASTNode {
	std::string            name;
	std::vector<ParamNode> params;
	bool                   returnsVoid = true;
	bool                   returnsArray = false;
	int                    returnSize = 0;
	std::vector<NodePtr>   body;
};

struct FuncCallNode : ASTNode {
	std::string          name;
	std::vector<NodePtr> args;
};

struct InNode : ASTNode {};