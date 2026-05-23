#pragma once
#include <vector>
#include "token.h"
#include "ast.h"

class Parser {
	public:
		Parser(const std::vector<Token>& tokens);
		std::vector<NodePtr> parse();

	private:
		std::vector<Token> tokens;
		int pos;

		Token& current();
		Token& peek(int offset);
		Token& advance();
		bool check(TokenType type);
		bool match(TokenType type);
		Token expect(TokenType type, const std::string& msg);

		NodePtr parseStatement();
		NodePtr parseVarDecl();
		NodePtr parseArrayDecl();
		NodePtr parseAssign();
		NodePtr parseIf();
		NodePtr parseWhile();
		NodePtr parseReturn();
		NodePtr parsePrint();
		NodePtr parseExpression();
		NodePtr parseComparison();
		NodePtr parseBitOr();
		NodePtr parseBitXor();
		NodePtr parseBitAnd();
		NodePtr parseShift();
		NodePtr parseUnary();
		NodePtr parsePrimary();
};