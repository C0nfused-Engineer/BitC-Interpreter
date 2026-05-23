#include "parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

Token& Parser::current() { return tokens[pos]; }

Token& Parser::peek(int offset) {
	int i = pos + offset;
	if (i >= (int)tokens.size()) return tokens.back();
	return tokens[i];
}

Token& Parser::advance() {
	Token& t = tokens[pos];
	if (pos < (int)tokens.size() - 1) pos++;
	return t;
}

bool Parser::check(TokenType type) {
	return current().type == type;
}

bool Parser::match(TokenType type) {
	if (check(type)) { advance(); return true; }
	return false;
}

Token Parser::expect(TokenType type, const std::string& msg) {
	if (!check(type)) {
		throw std::runtime_error(msg + " at line " + std::to_string(current().line)
			+ " col " + std::to_string(current().col)
			+ " (got '" + current().text + "')");
	}
	return advance();
}

std::vector<NodePtr> Parser::parse() {
	std::vector<NodePtr> statements;
	while (!check(TokenType::END_OF_FILE)) {
		statements.push_back(parseStatement());
	}
	return statements;
}

NodePtr Parser::parseStatement() {
	// Step 11: reserved keyword guards — error immediately, do not parse further
	if (check(TokenType::NULL_KW) || check(TokenType::OVERLOAD) ||
		check(TokenType::PTR) || check(TokenType::ALIAS)) {
		throw std::runtime_error("keyword '" + current().text
			+ "' is reserved and not yet implemented at line "
			+ std::to_string(current().line)
			+ " col " + std::to_string(current().col));
	}
	if (check(TokenType::FOR)) {
		throw std::runtime_error("keyword 'for' is not yet implemented at line "
			+ std::to_string(current().line)
			+ " col " + std::to_string(current().col));
	}

	// Step 12: function declarations
	if (check(TokenType::FN)) return parseFuncDecl();

	if (check(TokenType::BIT)) return parseVarDecl();
	if (check(TokenType::IF))  return parseIf();
	if (check(TokenType::WHILE)) return parseWhile();
	if (check(TokenType::RETURN)) return parseReturn();
	if (check(TokenType::PRINT)) return parsePrint();

	if (check(TokenType::BREAK)) {
		advance();
		expect(TokenType::SEMICOLON, "expected ';' after break");
		return std::make_unique<BreakNode>();
	}
	if (check(TokenType::CONTINUE)) {
		advance();
		expect(TokenType::SEMICOLON, "expected ';' after continue");
		return std::make_unique<ContinueNode>();
	}

	// Step 15: IDENTIFIER — either a function call statement or an assignment.
	// Peek ahead: IDENTIFIER followed by '(' is a call; followed by '=' is assign.
	if (check(TokenType::IDENTIFIER)) {
		if (peek(1).type == TokenType::LPAREN) {
			int ln = current().line, cl = current().col;
			std::string name = advance().text; // consume the identifier
			NodePtr call = parseFuncCall(name, ln, cl);
			expect(TokenType::SEMICOLON, "expected ';' after function call");
			return call;
		}
		return parseAssign();
	}

	throw std::runtime_error("unexpected token '" + current().text
		+ "' at line " + std::to_string(current().line)
		+ " col " + std::to_string(current().col));
}

// -----------------------------------------------------------------------
// Variable / array declarations
// -----------------------------------------------------------------------

NodePtr Parser::parseVarDecl() {
	int ln = current().line, cl = current().col;
	expect(TokenType::BIT, "expected 'bit'");

	if (check(TokenType::LBRACKET)) {
		return parseArrayDecl();
	}

	Token name = expect(TokenType::IDENTIFIER, "expected variable name");
	expect(TokenType::EQUALS, "expected '='");
	NodePtr init = parseExpression();
	expect(TokenType::SEMICOLON, "expected ';'");

	auto node = std::make_unique<VarDeclNode>();
	node->line = ln; node->col = cl;
	node->name = name.text;
	node->initializer = std::move(init);
	return node;
}

NodePtr Parser::parseArrayDecl() {
	// 'bit' already consumed, now at '['
	int ln = current().line, cl = current().col;
	expect(TokenType::LBRACKET, "expected '['");
	Token sizeToken = expect(TokenType::NUMBER, "expected array size");
	expect(TokenType::RBRACKET, "expected ']'");

	Token name = expect(TokenType::IDENTIFIER, "expected array name");
	expect(TokenType::EQUALS, "expected '='");
	NodePtr init = parseExpression();
	expect(TokenType::SEMICOLON, "expected ';'");

	auto node = std::make_unique<ArrayDeclNode>();
	node->line = ln; node->col = cl;
	node->name = name.text;
	node->size = std::stoi(sizeToken.text);
	node->initializer = std::move(init);
	return node;
}

// -----------------------------------------------------------------------
// Step 13: Function declaration
// Grammar: fn NAME ( paramList ) -> returnType { body }
//       or fn NAME ( paramList )              { body }   (void)
// -----------------------------------------------------------------------

NodePtr Parser::parseFuncDecl() {
	int ln = current().line, cl = current().col;
	expect(TokenType::FN, "expected 'fn'");

	Token name = expect(TokenType::IDENTIFIER, "expected function name");
	expect(TokenType::LPAREN, "expected '(' after function name");

	// Parameter list
	std::vector<ParamNode> params;
	while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
		ParamNode param;
		expect(TokenType::BIT, "expected 'bit' in parameter type");

		if (check(TokenType::LBRACKET)) {
			expect(TokenType::LBRACKET, "expected '['");
			Token sz = expect(TokenType::NUMBER, "expected array size in parameter type");
			expect(TokenType::RBRACKET, "expected ']'");
			param.isArray = true;
			param.arraySize = std::stoi(sz.text);
		}

		Token pname = expect(TokenType::IDENTIFIER, "expected parameter name");
		param.name = pname.text;
		params.push_back(param);

		if (!check(TokenType::RPAREN)) {
			expect(TokenType::COMMA, "expected ',' between parameters");
		}
	}
	expect(TokenType::RPAREN, "expected ')'");

	// Optional return type: -> bit  or  -> bit[N]
	auto node = std::make_unique<FuncDeclNode>();
	node->line = ln; node->col = cl;
	node->name = name.text;
	node->params = std::move(params);

	if (match(TokenType::ARROW)) {
		expect(TokenType::BIT, "expected 'bit' as return type after '->'");
		node->returnsVoid = false;
		if (check(TokenType::LBRACKET)) {
			expect(TokenType::LBRACKET, "expected '['");
			Token sz = expect(TokenType::NUMBER, "expected return array size");
			expect(TokenType::RBRACKET, "expected ']'");
			node->returnsArray = true;
			node->returnSize = std::stoi(sz.text);
		}
	}
	// else returnsVoid stays true (default)

	// Body
	expect(TokenType::LBRACE, "expected '{' before function body");
	while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
		node->body.push_back(parseStatement());
	}
	expect(TokenType::RBRACE, "expected '}' after function body");

	// No semicolon after a function declaration
	return node;
}

// -----------------------------------------------------------------------
// Step 14: Function call (called from parsePrimary and parseStatement)
// The identifier has already been consumed by the caller.
// -----------------------------------------------------------------------

NodePtr Parser::parseFuncCall(const std::string& name, int ln, int cl) {
	expect(TokenType::LPAREN, "expected '(' in function call");

	auto node = std::make_unique<FuncCallNode>();
	node->line = ln; node->col = cl;
	node->name = name;

	while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
		node->args.push_back(parseExpression());
		if (!check(TokenType::RPAREN)) {
			expect(TokenType::COMMA, "expected ',' between arguments");
		}
	}
	expect(TokenType::RPAREN, "expected ')' after arguments");

	return node;
}

NodePtr Parser::parseAssign() {
	int ln = current().line, cl = current().col;
	Token name = expect(TokenType::IDENTIFIER, "expected identifier");
	expect(TokenType::EQUALS, "expected '='");
	NodePtr val = parseExpression();
	expect(TokenType::SEMICOLON, "expected ';'");

	auto node = std::make_unique<AssignNode>();
	node->line = ln; node->col = cl;
	node->name = name.text;
	node->value = std::move(val);
	return node;
}

NodePtr Parser::parseIf() {
	int ln = current().line, cl = current().col;
	expect(TokenType::IF, "expected 'if'");
	expect(TokenType::LPAREN, "expected '('");
	NodePtr cond = parseExpression();
	expect(TokenType::RPAREN, "expected ')'");
	expect(TokenType::LBRACE, "expected '{'");

	std::vector<NodePtr> thenBody;
	while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
		thenBody.push_back(parseStatement());
	}
	expect(TokenType::RBRACE, "expected '}'");

	std::vector<NodePtr> elseBody;
	if (match(TokenType::ELSE)) {
		expect(TokenType::LBRACE, "expected '{'");
		while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
			elseBody.push_back(parseStatement());
		}
		expect(TokenType::RBRACE, "expected '}'");
	}

	auto node = std::make_unique<IfNode>();
	node->line = ln; node->col = cl;
	node->condition = std::move(cond);
	node->thenBody = std::move(thenBody);
	node->elseBody = std::move(elseBody);
	return node;
}

NodePtr Parser::parseWhile() {
	int ln = current().line, cl = current().col;
	expect(TokenType::WHILE, "expected 'while'");
	expect(TokenType::LPAREN, "expected '('");
	NodePtr cond = parseExpression();
	expect(TokenType::RPAREN, "expected ')'");
	expect(TokenType::LBRACE, "expected '{'");

	std::vector<NodePtr> body;
	while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
		body.push_back(parseStatement());
	}
	expect(TokenType::RBRACE, "expected '}'");

	auto node = std::make_unique<WhileNode>();
	node->line = ln; node->col = cl;
	node->condition = std::move(cond);
	node->body = std::move(body);
	return node;
}

NodePtr Parser::parseReturn() {
	int ln = current().line, cl = current().col;
	expect(TokenType::RETURN, "expected 'return'");

	auto node = std::make_unique<ReturnNode>();
	node->line = ln; node->col = cl;

	if (!check(TokenType::SEMICOLON)) {
		node->value = parseExpression();
	}
	expect(TokenType::SEMICOLON, "expected ';'");
	return node;
}

// Step 16: print now requires parentheses — print(expr);
NodePtr Parser::parsePrint() {
	int ln = current().line, cl = current().col;
	expect(TokenType::PRINT, "expected 'print'");
	expect(TokenType::LPAREN, "expected '(' after 'print'");
	NodePtr val = parseExpression();
	expect(TokenType::RPAREN, "expected ')' after print expression");
	expect(TokenType::SEMICOLON, "expected ';'");

	auto node = std::make_unique<PrintNode>();
	node->line = ln; node->col = cl;
	node->value = std::move(val);
	return node;
}

// Step 14: in() — parsed as a primary expression, no arguments
NodePtr Parser::parseIn() {
	int ln = current().line, cl = current().col;
	expect(TokenType::IN, "expected 'in'");
	expect(TokenType::LPAREN, "expected '(' after 'in'");
	expect(TokenType::RPAREN, "expected ')' — in() takes no arguments");

	auto node = std::make_unique<InNode>();
	node->line = ln; node->col = cl;
	return node;
}

// -----------------------------------------------------------------------
// Expressions (precedence climbing — lowest to highest)
// -----------------------------------------------------------------------

NodePtr Parser::parseExpression() {
	return parseComparison();
}

NodePtr Parser::parseComparison() {
	NodePtr left = parseBitOr();

	while (check(TokenType::EQ_EQ) || check(TokenType::EXC_EQ) ||
		check(TokenType::LANGLE) || check(TokenType::RANGLE)) {
		std::string op = advance().text;
		NodePtr right = parseBitOr();

		auto node = std::make_unique<BinaryOpNode>();
		node->op = op;
		node->left = std::move(left);
		node->right = std::move(right);
		left = std::move(node);
	}
	return left;
}

NodePtr Parser::parseBitOr() {
	NodePtr left = parseBitXor();

	while (check(TokenType::PIPE)) {
		std::string op = advance().text;
		NodePtr right = parseBitXor();

		auto node = std::make_unique<BinaryOpNode>();
		node->op = op;
		node->left = std::move(left);
		node->right = std::move(right);
		left = std::move(node);
	}
	return left;
}

NodePtr Parser::parseBitXor() {
	NodePtr left = parseBitAnd();

	while (check(TokenType::CARET)) {
		std::string op = advance().text;
		NodePtr right = parseBitAnd();

		auto node = std::make_unique<BinaryOpNode>();
		node->op = op;
		node->left = std::move(left);
		node->right = std::move(right);
		left = std::move(node);
	}
	return left;
}

NodePtr Parser::parseBitAnd() {
	NodePtr left = parseShift();

	while (check(TokenType::AMP)) {
		std::string op = advance().text;
		NodePtr right = parseShift();

		auto node = std::make_unique<BinaryOpNode>();
		node->op = op;
		node->left = std::move(left);
		node->right = std::move(right);
		left = std::move(node);
	}
	return left;
}

NodePtr Parser::parseShift() {
	NodePtr left = parseUnary();

	while (check(TokenType::LSHIFT) || check(TokenType::RSHIFT)) {
		std::string op = advance().text;
		NodePtr right = parseUnary();

		auto node = std::make_unique<BinaryOpNode>();
		node->op = op;
		node->left = std::move(left);
		node->right = std::move(right);
		left = std::move(node);
	}
	return left;
}

NodePtr Parser::parseUnary() {
	if (check(TokenType::TILDE) || check(TokenType::EXC)) {
		std::string op = advance().text;
		NodePtr operand = parseUnary(); // right-associative

		auto node = std::make_unique<UnaryOpNode>();
		node->op = op;
		node->operand = std::move(operand);
		return node;
	}
	return parsePrimary();
}

NodePtr Parser::parsePrimary() {
	// parenthesised expression
	if (check(TokenType::LPAREN)) {
		advance();
		NodePtr expr = parseExpression();
		expect(TokenType::RPAREN, "expected ')'");
		return expr;
	}

	// array literal {1, 0, 1, 0}
	if (check(TokenType::LBRACE)) {
		int ln = current().line, cl = current().col;
		advance();
		auto node = std::make_unique<ArrayLiteralNode>();
		node->line = ln; node->col = cl;

		if (!check(TokenType::RBRACE)) {
			node->elements.push_back(parseExpression());
			while (match(TokenType::COMMA)) {
				node->elements.push_back(parseExpression());
			}
		}
		expect(TokenType::RBRACE, "expected '}'");
		return node;
	}

	// in() expression
	if (check(TokenType::IN)) {
		return parseIn();
	}

	// bit literal: 0, 1, true, false
	if (check(TokenType::BIT_LITERAL)) {
		int ln = current().line, cl = current().col;
		Token t = advance();
		auto node = std::make_unique<BitLiteralNode>();
		node->line = ln; node->col = cl;
		node->value = (t.text == "1" || t.text == "true") ? 1 : 0;
		return node;
	}

	// number (array size context)
	if (check(TokenType::NUMBER)) {
		int ln = current().line, cl = current().col;
		Token t = advance();
		auto node = std::make_unique<NumberNode>();
		node->line = ln; node->col = cl;
		node->value = std::stoi(t.text);
		return node;
	}

	// identifier or function call
	if (check(TokenType::IDENTIFIER)) {
		int ln = current().line, cl = current().col;
		Token t = advance();

		// Step 14: if followed by '(' it's a call expression (e.g. used as rvalue)
		if (check(TokenType::LPAREN)) {
			return parseFuncCall(t.text, ln, cl);
		}

		auto node = std::make_unique<IdentifierNode>();
		node->line = ln; node->col = cl;
		node->name = t.text;
		return node;
	}

	throw std::runtime_error("unexpected token '" + current().text
		+ "' at line " + std::to_string(current().line)
		+ " col " + std::to_string(current().col));
}