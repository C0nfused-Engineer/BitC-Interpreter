#include "parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

Token& Parser::current() { return tokens[pos]; }

Token& Parser::peek(int offset) {
	int i = pos + offset;
	if (i >= (int)tokens.size()) return tokens.back();
	return  tokens[i];
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
	if (check(TokenType::BIT)) return parseVarDecl();
	if (check(TokenType::IF)) return parseIf();
	if (check(TokenType::WHILE)) return parseWhile();
	if (check(TokenType::RETURN)) return parseReturn();
	if (check(TokenType::PRINT)) return parsePrint();
	if (check(TokenType::BREAK)) {
		advance();
		expect(TokenType::SEMICOLON, "expected ';' after break");
		auto node = std::make_unique<BreakNode>();
		return node;
	}
	if (check(TokenType::CONTINUE)) {
		advance();
		expect(TokenType::SEMICOLON, "expected ';' after continue");
		auto node = std::make_unique<ContinueNode>();
		return node;
	}

	return parseAssign();
}

NodePtr Parser::parseVarDecl() {
    int ln = current().line, cl = current().col;
    expect(TokenType::BIT, "expected 'bit'");

    // check for bit[N]
    if (check(TokenType::LBRACKET)) {
        return parseArrayDecl();
    }

    // plain bit x = expr;
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
    // we already consumed 'bit', now at '['
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

NodePtr Parser::parsePrint() {
    int ln = current().line, cl = current().col;
    expect(TokenType::PRINT, "expected 'print'");
    NodePtr val = parseExpression();
    expect(TokenType::SEMICOLON, "expected ';'");

    auto node = std::make_unique<PrintNode>();
    node->line = ln; node->col = cl;
    node->value = std::move(val);
    return node;
}

// ??? expressions (precedence climbing) ???????????????????????????????????????
//
// Lowest precedence at the top, highest at the bottom.
// Each level calls the next level for its operands.

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
        NodePtr operand = parseUnary();  // right-associative

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

    // array literal {1,0,1,0}
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

    // bit literal
    if (check(TokenType::BIT_LITERAL)) {
        int ln = current().line, cl = current().col;
        Token t = advance();
        auto node = std::make_unique<BitLiteralNode>();
        node->line = ln; node->col = cl;
        node->value = (t.text == "1" || t.text == "true") ? 1 : 0;
        return node;
    }

    // number (array size context, or bare number)
    if (check(TokenType::NUMBER)) {
        int ln = current().line, cl = current().col;
        Token t = advance();
        auto node = std::make_unique<NumberNode>();
        node->line = ln; node->col = cl;
        node->value = std::stoi(t.text);
        return node;
    }

    // identifier
    if (check(TokenType::IDENTIFIER)) {
        int ln = current().line, cl = current().col;
        Token t = advance();
        auto node = std::make_unique<IdentifierNode>();
        node->line = ln; node->col = cl;
        node->name = t.text;
        return node;
    }

    throw std::runtime_error("Unexpected token '" + current().text
        + "' at line " + std::to_string(current().line));
}