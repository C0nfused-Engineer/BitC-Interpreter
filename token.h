#pragma once
#include <string>

enum class TokenType {
	// Literals
	BIT_LITERAL, // 0 or 1, TRUE or FALSE
	NUMBER, // digit for sizing bit arrays

	// Keywords
	BIT,         // bit
	VOID,        // void
	IF,          // if
	ELSE,        // else
	WHILE,       // while
	RETURN,      // return
	BREAK,       // break
	CONTINUE,    // continue
	INCLUDE,     // include
	PRINT,       // print

	// Identifiers
	IDENTIFIER,  // variable name

	// Operator and Punctuation
	EQUALS,      // =
	EQ_EQ,       // ==
	AMP,         // &
	AMP_AMP,     // &&
	AMP_EQ,      // &=
	PIPE,        // |
	PIPE_PIPE,   // ||
	PIPE_EQ,     // |=
	CARET,       // ^
	ASTR,        // *
	TILDE,       // ~
	LSHIFT,      // <<
	RSHIFT,      // >>
	HASH,        // #
	DOT,         // .
	EXC,         // !
	EXC_EQ,      // !=
	ARROW,       // ->
	SEMICOLON,   // ;
	COLON,       // :
	COLON_COLON, // ::
	SLASH,       // /
	SLASH_SLASH, // //
	LANGLE,      // <
	RANGLE,      // >
	LPAREN,      // (
	RPAREN,      // )
	LBRACE,      // {
	RBRACE,      // }
	LBRACKET,    // [
	RBRACKET,    // ]
	COMMA,       // ,

	END_OF_FILE
};

struct Token {
	TokenType type;
	std::string text;
	int line;
	int col;
};