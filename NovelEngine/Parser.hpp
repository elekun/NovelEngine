#pragma once
#include "Token.hpp"

class Parser {
public:
	Parser();
	Parser& init(Array<shared_ptr<Token>> ts);
	Array<shared_ptr<Token>> block();
	static shared_ptr<Token> blank;
private:
	HashTable<String, int32> degrees;
	Array<String> factorKinds;
	Array<String> binaryKinds;
	Array<String> rightAssocs;
	Array<String> unaryOperators;
	Array<String> reserved;

	Array<shared_ptr<Token>> tokens;
	int32 i;
	shared_ptr<Token> token();
	shared_ptr<Token> next();
	shared_ptr<Token> lead(shared_ptr<Token> token);
	int32 degree(shared_ptr<Token> t);
	shared_ptr<Token> expression(int32 leftDegree);
	shared_ptr<Token> bind(shared_ptr<Token> left, shared_ptr<Token> op);
	shared_ptr<Token> consume(String expectedValue);
	shared_ptr<Token> func(shared_ptr<Token> token);
	shared_ptr<Token> ident();
	shared_ptr<Token> if_(shared_ptr<Token> token);
	Array<shared_ptr<Token>> body();
	shared_ptr<Token> while_(shared_ptr<Token> token);
	shared_ptr<Token> var(shared_ptr<Token> token);
	shared_ptr<Token> newArray(shared_ptr<Token> token);
};

