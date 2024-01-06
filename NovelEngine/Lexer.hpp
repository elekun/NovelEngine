#pragma once
#include "Token.hpp"

using namespace std;

class Lexer {
public:
	Lexer() {};
	Lexer& init(String t);

	shared_ptr<Token> nextToken();
	Array<shared_ptr<Token>> tokenize();

private:
	String text; //解析するテキスト
	int32 i; //何文字目まで読んだか

	bool isEOT(); //終端まで来たかどうか
	char32 nowc(); //現在見ている文字
	char32 next(); //見ている文字を返して次に行く
	void skipSpace(); //スペースを飛ばす
	bool isSignStart(char32 c);
	bool isNumberStart(char32 c);
	bool isIdentStart(char32 c);
	bool isParenStart(char32 c);
	bool isCurlyStart(char32 c);
	bool isSymbolStart(char32 c);
	bool isStringStart(char32 c);
	bool isBracketStart(char32 c);
	shared_ptr<Token> sign();
	shared_ptr<Token> number();
	shared_ptr<Token> ident();
	shared_ptr<Token> paren();
	shared_ptr<Token> curly();
	shared_ptr<Token> symbol();
	shared_ptr<Token> str();
	shared_ptr<Token> bracket();
};

