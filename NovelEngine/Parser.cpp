#include "stdafx.h"
#include "Parser.hpp"

Parser::Parser() {
	degrees = {};
	degrees.emplace(U"(", 80);
	degrees.emplace(U"*", 60);
	degrees.emplace(U"/", 60);
	degrees.emplace(U"+", 50);
	degrees.emplace(U"-", 50);
	degrees.emplace(U"==", 40);
	degrees.emplace(U"!=", 40);
	degrees.emplace(U"<", 40);
	degrees.emplace(U"<=", 40);
	degrees.emplace(U">", 40);
	degrees.emplace(U">=", 40);
	degrees.emplace(U"&&", 30);
	degrees.emplace(U"||", 30);
	degrees.emplace(U"=", 10);

	factorKinds = { U"digit", U"ident", U"string"};
	binaryKinds = { U"sign" };
	rightAssocs = { U"=" };
	unaryOperators = { U"+", U"-", U"!" };
	reserved = { U"function",U"return", U"if", U"else", U"while", U"break", U"var"};
}

Parser& Parser::init(Array<shared_ptr<Token>> ts) {
	i = 0;
	tokens = ts;
	tokens << make_shared<Token>(Token{ U"eob", U"(eob)" });
	return *this;
}

Array<shared_ptr<Token>> Parser::block() {
	Array<shared_ptr<Token>> blk = {};
	while (token()->kind != U"eob") {
		blk << expression(0);
	}
	return blk;
}


shared_ptr<Token> Parser::token() {
	if (tokens.size() <= i) {
		throw Error{U"No more token"};
	}
	return tokens[i];
}

shared_ptr<Token> Parser::next() {
	auto t = token();
	++i;
	return t;
}

shared_ptr<Token> Parser::lead(shared_ptr<Token> token) {
	if (token->kind == U"ident" && token->value == U"function") {
		return func(token);
	}
	else if (token->kind == U"ident" && token->value == U"return") {
		token->kind = U"ret";
		if (this->token()->kind != U"eob") {
			token->left = expression(0);
		}
		return token;
	}
	else if (token->kind == U"ident" && token->value == U"if") {
		return if_(token);
	}
	else if (token->kind == U"ident" && token->value == U"while") {
		return while_(token);
	}
	else if (token->kind == U"ident" && token->value == U"break") {
		token->kind = U"brk";
		return token;
	}
	else if (token->kind == U"ident" && token->value == U"var") {
		return var(token);
	}
	else if (factorKinds.contains(token->kind)) {
		return token;
	}
	else if (unaryOperators.contains(token->value)) {
		token->kind = U"unary";
		token->left = expression(70);
		return token;
	}
	else if (token->kind == U"paren" && token->value == U"(") {
		shared_ptr<Token> expr = expression(0);
		consume(U")");
		return expr;
	}
	else {
		throw Error{U"The token can't place there"};
	}
}

int32 Parser::degree(shared_ptr<Token> t) {
	return degrees.contains(t->value) ? degrees[t->value] : 0;
}

shared_ptr<Token> Parser::expression(int32 leftDegree) {
	auto left = lead(next());
	int32 rightDegree = degree(token());
	while (leftDegree < rightDegree) {
		auto op = next();
		left = bind(left, op);
		rightDegree = degree(token());
	}
	return left;
}

shared_ptr<Token> Parser::bind(shared_ptr<Token> left, shared_ptr<Token> op) {
	if (binaryKinds.contains(op->kind)) {
		op->left = left;
		int32 leftDegree = degree(op);
		if (rightAssocs.contains(op->value)) {
			leftDegree--;
		}
		op->right = expression(leftDegree);
		return op;
	}
	else if (op->kind == U"paren" && op->value == U"(") {
		op->left = left;
		op->params = {};
		if (token()->value != U")") {
			op->params << expression(0);
			while (token()->value != U")") {
				consume(U",");
				op->params << expression(0);
			}
		}
		consume(U")");
		return op;
	}
	else {
		throw Error{U"The token can't place there"};
	}
}

shared_ptr<Token> Parser::consume(String expectedValue) {
	if (expectedValue != token()->value) {
		throw Error{U"Not expected value"};
	}
	return next();
}

shared_ptr<Token> Parser::func(shared_ptr<Token> token) {
	if (this->token()->value == U"(") {
		token->kind = U"fexpr";
	}
	else {
		token->kind = U"func";
		token->ident = ident();
	}
	consume(U"(");
	token->params = {};
	if (this->token()->value != U")") {
		token->params << ident();
		while (this->token()->value != U")") {
			consume(U",");
			token->params << ident();
		}
	}
	consume(U")");
	token->block = body();
	return token;
}

shared_ptr<Token> Parser::ident() {
	shared_ptr<Token> id = next();
	if (id->kind != U"ident") {
		throw Error{U"Not an identical token."};
	}
	if (reserved.contains(id->value)) {
		throw Error{U"The token was reserved."};
	}
	return id;
}

shared_ptr<Token> Parser::if_(shared_ptr<Token> token) {
	token->kind = U"if";
	consume(U"(");
	token->left = expression(0);
	consume(U")");
	if (this->token()->value == U"{") {
		token->block = body();
	}
	else {
		token->block = {};
		token->block << expression(0);
	}

	if (this->token()->value == U"else") {
		consume(U"else");
		if (this->token()->value == U"{") {
			token->blockOfElse = body();
		}
		else {
			token->blockOfElse = {};
			token->blockOfElse << expression(0);
		}
	}

	return token;
}

Array<shared_ptr<Token>> Parser::body() {
	consume(U"{");
	auto b = block();
	consume(U"}");
	return b;
}

shared_ptr<Token> Parser::while_(shared_ptr<Token> token) {
	token->kind = U"while";
	consume(U"(");
	token->left = expression(0);
	consume(U")");
	if (this->token()->value == U"{") {
		token->block = body();
	}
	else {
		token->block = {};
		token->block << expression(0);
	}
	return token;
}

shared_ptr<Token> Parser::var(shared_ptr<Token> token) {
	token->kind = U"var";
	token->block = {};
	shared_ptr<Token> item;
	shared_ptr<Token> ident = this->ident();
	if (this->token()->value == U"=") {
		shared_ptr<Token> op = next();
		item = bind(ident, op);
	}
	else {
		item = ident;
	}
	token->block << item;

	while (this->token()->value == U",") {
		next();
		ident = this->ident();
		if (this->token()->value == U"=") {
			shared_ptr<Token> op = next();
			item = bind(ident, op);
		}
		else {
			item = ident;
		}
		token->block << item;
	}
	return token;
}

