#pragma once

using namespace std;

class Token {
public:
	Token(String k, String v) : kind(k), value(v), left(nullptr), center(nullptr), right(nullptr), ident(nullptr), params({}), block({}), blockOfElse({}) {};

	String toString();
	String paren();

	String kind;
	String value;
	shared_ptr<Token> left;
	shared_ptr<Token> center;
	shared_ptr<Token> right;
	shared_ptr<Token> ident;
	Array<shared_ptr<Token>> params;
	Array<shared_ptr<Token>> block;
	Array<shared_ptr<Token>> blockOfElse;
private:
};

