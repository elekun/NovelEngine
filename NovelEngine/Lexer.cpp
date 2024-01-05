#include "stdafx.h"
#include "Lexer.hpp"

Lexer& Lexer::init(String t) {
	i = 0;
	text = t;
	return *this;
}

shared_ptr<Token> Lexer::nextToken() {
	skipSpace();
	if (isEOT()) {
		return nullptr;
	}
	else if (isSignStart(nowc())) {
		return sign();
	}
	else if (isDigitStart(nowc())) {
		return digit();
	}
	else if (isStringStart(nowc())) {
		return str();
	}
	else if (isIdentStart(nowc())) {
		return ident();
	}
	else if (isParenStart(nowc())) {
		return paren();
	}
	else if (isCurlyStart(nowc())) {
		return curly();
	}
	else if (isBracketStart(nowc())) {
		return bracket();
	}
	else if (isSymbolStart(nowc())) {
		return symbol();
	}
	else {
		throw Error{U"Not a character for tokens"};
	}
}

Array<shared_ptr<Token>> Lexer::tokenize() {
	Array<shared_ptr<Token>> tokens = {};
	auto t = nextToken();
	while (t) {
		tokens << t;
		t = nextToken();
	}
	return tokens;
}

bool Lexer::isEOT() {
	return text.length() <= i;
}

char32 Lexer::nowc() {
	if (isEOT()) {
		throw Error{U"No more character"};
	}
	return text[i];
}

char32 Lexer::next() {
	auto c = nowc();
	++i;
	return c;
}

void Lexer::skipSpace() {
	std::locale loc;
	while (!isEOT() && std::isblank<char>(nowc(), loc)) {
		next();
	}
}

bool Lexer::isSignStart(char32 c) {
	return c == U'=' || c == U'+' || c == U'-' || c == U'*' || c == U'/' || c == U'!' || c == U'<' || c == U'>' || c == U'&' || c == U'|';
}

bool Lexer::isDigitStart(char32 c) {
	std::locale loc;
	return std::isdigit<char>(c, loc);
}

bool Lexer::isIdentStart(char32 c) {
	std::locale loc;
	return std::isalpha<char>(c, loc) || c == U'_';
}

bool Lexer::isParenStart(char32 c) {
	return c == U'(' || c == U')';
}

bool Lexer::isCurlyStart(char32 c) {
	return c == U'{' || c == U'}';
}

bool Lexer::isSymbolStart(char32 c) {
	return c == U',';
}

bool Lexer::isStringStart(char32 c) {
	return c == U'"';
}

bool Lexer::isBracketStart(char32 c) {
	return c == U'[' || c == U']';
}

shared_ptr<Token> Lexer::sign() {
	Token t = Token{ U"sign", U"" };
	char32 c1 = next();
	char32 c2 = (char32)0;
	if (!isEOT()) {
		if (c1 == U'=' || c1 == U'!' || c1 == U'<' || c1 == U'>') {
			if (nowc() == U'=') {
				c2 = next();
			}
		}
		else if (c1 == U'&') {
			if (nowc() == U'&') {
				c2 = next();
			}
		}
		else if (c1 == U'|') {
			if (nowc() == U'|') {
				c2 = next();
			}
		}
	}
	String v;
	if (c2 == (char32)0) {
		v = String{ c1 };
	}
	else {
		v = String{ c1 } + String{ c2 };
	}
	t.value = v;
	return make_shared<Token>(t);
}

shared_ptr<Token> Lexer::digit() {
	String s = U"";
	s << next();
	std::locale loc;
	while (!isEOT() && std::isdigit<char>(nowc(), loc)) {
		s << next();
	}
	return make_shared<Token>(Token{ U"digit", s });
}

shared_ptr<Token> Lexer::ident() {
	String s = U"";
	s << next();
	std::locale loc;
	while (!isEOT() && (std::isdigit<char>(nowc(), loc) || std::isalpha<char>(nowc(), loc) || nowc() == U'_')) {
		s << next();
	}
	return make_shared<Token>(Token{ U"ident", s });
}

shared_ptr<Token> Lexer::paren() {
	return make_shared<Token>(Token{ U"paren", String{next()} });
}

shared_ptr<Token> Lexer::curly() {
	String k = U"";
	if (nowc() == U'{') {
		k = U"curly";
	}
	else {
		k = U"eob";
	}
	return make_shared<Token>(Token{ k, String{next()} });
}

shared_ptr<Token> Lexer::symbol() {
	return make_shared<Token>(Token{ U"symbol", String{next()} });
}

shared_ptr<Token> Lexer::str() {
	String s = U"";
	next();
	while (nowc() != U'"') {
		if (nowc() != U'\\') {
			s += next();
		}
		else {
			next();
			char32 c = nowc();
			if (c == U'"') {
				s += U'"';
				next();
			}
			else if (c == U'\\') {
				s += U'\\';
				next();
			}
			else if (c == U'/') {
				s += U'/';
				next();
			}
			else if (c == U'b') {
				s += U'\b';
				next();
			}
			else if (c == U'f') {
				s += U'\f';
				next();
			}
			else if (c == U'n') {
				s += U'\n';
				next();
			}
			else if (c == U'r') {
				s += U'\r';
				next();
			}
			else if (c == U'\t') {
				s += U'\t';
				next();
			}
			else {
				throw Error{U"string error"};
			}
		}
	}
	next();
	return make_shared<Token>(Token{ U"string", s });
}

shared_ptr<Token> Lexer::bracket() {
	return make_shared<Token>(Token{ U"bracket", String{next()} });
}

