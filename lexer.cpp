#include "lexer.h"
#include <stdexcept>
#include <unordered_map>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), col(1) {}

char Lexer::current() {
	if (pos >= (int)source.size()) return '\0';
	return source[pos];
}

char Lexer::peek(int offset) {
	int i = pos + offset;
	if (i >= (int)source.size()) return '\0';
	return source[i];
}

char Lexer::advance() {
	char c = source[pos++];
	if (c == '\n') {
		line++;
		col = 1;
	} else {
		col++;
	}

	return c;
}

Token Lexer::makeToken(TokenType type, const std::string& text, int startCol) {
	return { type, text, line, startCol };
}

void Lexer::skipWhitespace() {
	while (pos < (int)source.size() && isspace(current())) {
		advance();
	}
}

void Lexer::skipLineComment() {
	while (pos < (int)source.size() && current() != '\n') {
		advance();
	}
}

Token Lexer::readIdentKey() {
	int startCol = col;
	std::string text;

	while (pos < (int)source.size() && (isalnum(current()) || current() == '_')) {
		text += advance();
	}

	static const std::unordered_map<std::string, TokenType> keywords = {
		{"bit",      TokenType::BIT},
		{"void",     TokenType::VOID},
		{"if",       TokenType::IF},
		{"else",     TokenType::ELSE},
		{"while",    TokenType::WHILE},
		{"return",   TokenType::RETURN},
		{"break",    TokenType::BREAK},
		{"for",    TokenType::FOR},
		{"continue", TokenType::CONTINUE},
		{"include",  TokenType::INCLUDE},
		{"print",    TokenType::PRINT},
		{"true",     TokenType::BIT_LITERAL},
		{"false",    TokenType::BIT_LITERAL},
		{"NULL",     TokenType::NULL_KW},
		{"fn",       TokenType::FN},
		{"in",       TokenType::IN},
		{"overload", TokenType::OVERLOAD},
		{"ptr",      TokenType::PTR},
		{"alias",    TokenType::ALIAS},
	};

	auto it = keywords.find(text);
	TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
	return makeToken(type, text, startCol);
}

Token Lexer::readLiteral() {
	int startCol = col;
	std::string text(1, advance());
	return makeToken(TokenType::BIT_LITERAL, text, startCol);
}

Token Lexer::readNumber() {
	int startCol = col;
	std::string text;
	
	while (pos < (int)source.size() && isdigit(current())) {
		text += advance();
	}

	return makeToken(TokenType::NUMBER, text, startCol);
}

Token Lexer::readSymbol() {
	int startCol = col;
	char c = advance();

	switch(c) {
		case '&':
			if (current() == '&') { advance(); throw std::runtime_error("'&&' is not valid in BC26; use '&' for bitwise AND at line " + std::to_string(line) + " col " + std::to_string(startCol)); }
			if(current() == '=') { advance(); return makeToken(TokenType::AMP_EQ, "&=", startCol); }
			return makeToken(TokenType::AMP, "&", startCol);
		case '|':
			if (current() == '|') { advance(); throw std::runtime_error("'||' is not valid in BC26; use '|' for bitwise OR at line " + std::to_string(line) + " col " + std::to_string(startCol)); }
			if(current() == '=') { advance(); return makeToken(TokenType::PIPE_EQ, "|=", startCol); }
			return makeToken(TokenType::PIPE, "|", startCol);

		case '=':
			if(current() == '=') { advance(); return makeToken(TokenType::EQ_EQ, "==", startCol); }
			return makeToken(TokenType::EQUALS, "=", startCol);

		case '!':
			if(current() == '=') { advance(); return makeToken(TokenType::EXC_EQ, "!=", startCol); }
			return makeToken(TokenType::EXC, "!", startCol);

		case '<':
			if(current() == '<') { advance(); return makeToken(TokenType::LSHIFT, "<<", startCol); }
			return makeToken(TokenType::LANGLE, "<", startCol);

		case '>':
			if(current() == '>') { advance(); return makeToken(TokenType::RSHIFT, ">>", startCol); }
			return makeToken(TokenType::RANGLE, ">", startCol);

		case '-':
			if(current() == '>') { advance(); return makeToken(TokenType::ARROW, "->", startCol); }
			// bare minus not supported yet — fall through to error
			break;

		case '^': return makeToken(TokenType::CARET, "^", startCol);
		case '~': return makeToken(TokenType::TILDE, "~", startCol);
		case '#': return makeToken(TokenType::HASH, "#", startCol);
		case '.': return makeToken(TokenType::DOT, ".", startCol);
		case ';': return makeToken(TokenType::SEMICOLON, ";", startCol);
		case ',': return makeToken(TokenType::COMMA, ",", startCol);
		case '(': return makeToken(TokenType::LPAREN, "(", startCol);
		case ')': return makeToken(TokenType::RPAREN, ")", startCol);
		case '{': return makeToken(TokenType::LBRACE, "{", startCol);
		case '}': return makeToken(TokenType::RBRACE, "}", startCol);
		case '[': return makeToken(TokenType::LBRACKET, "[", startCol);
		case ']': return makeToken(TokenType::RBRACKET, "]", startCol);

		case ':':
			if(current() == ':') { advance(); return makeToken(TokenType::COLON_COLON, "::", startCol); }
			return makeToken(TokenType::COLON, ":", startCol);
	}

	throw std::runtime_error("Unexpected character '" + std::string(1, c)
		+ "' at line " + std::to_string(line)
		+ " col " + std::to_string(startCol));
}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> tokens;

	while(true) {
		skipWhitespace();

		if(pos >= (int)source.size()) {
			tokens.push_back(makeToken(TokenType::END_OF_FILE, "", col));
			break;
		}

		char c = current();

		if(c == '/' && peek(1) == '/') {
			skipLineComment();
			continue;
		}

		if(isalpha(c) || c == '_') {
			tokens.push_back(readIdentKey());
		} else if (c == '0' || c == '1') {
			tokens.push_back(readLiteral());
		} else if (isdigit(c)) {
			tokens.push_back(readNumber());
		} else {
			tokens.push_back(readSymbol());
		}
	}

	return tokens;
}