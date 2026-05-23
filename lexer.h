#pragma once
#include <string>
#include <vector>
#include "token.h"

class Lexer {
	public:
		Lexer(const std::string& source);
		std::vector<Token> tokenize();

	private:
		std::string source;
		int pos;
		int line;
		int col;

		char current();
		char peek(int offset);
		char advance();

		void skipWhitespace();
		void skipLineComment();

		Token readIdentKey();
		Token readLiteral();
		Token readNumber();
		Token readSymbol();

		Token makeToken(TokenType type, const std::string& text, int startCol);
};