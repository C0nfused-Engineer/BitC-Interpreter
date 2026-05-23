#pragma once
#include <string>

enum class TokenType {
	// Literals
	BIT_LITERAL, // 0 or 1, TRUE or FALSE
	NUMBER,      // digit for sizing bit arrays
	NULL_KW,     // NULL 

	// Keywords
	BIT,         // bit
	PTR,         // pointer
	VOID,        // void
	ALIAS,       // alias
	IF,          // if
	ELSE,        // else
	WHILE,       // while
	FOR,         // for ? might remove
	RETURN,      // return
	BREAK,       // break
	CONTINUE,    // continue
	INCLUDE,     // include
	IN,          // input
	PRINT,       // print
	FN,          // function
	OVERLOAD,    // overload

	// Identifiers
	IDENTIFIER,  // variable name

	// Operator and Punctuation
	EQUALS,      // =
	EQ_EQ,       // ==
	AMP,         // &
	AMP_EQ,      // &=
	PIPE,        // |
	PIPE_EQ,     // |=
	CARET,       // ^
	TILDE,       // ~
	LSHIFT,      // <<
	RSHIFT,      // >>
	ARROW,       // ->
	HASH,        // #
	DOT,         // .
	EXC,         // !
	EXC_EQ,      // !=
	SEMICOLON,   // ;
	COLON,       // :
	COLON_COLON, // ::
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