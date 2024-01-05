﻿#pragma once
#include <memory>
#include "GameScene.hpp"
#include "Token.hpp"
#include <any>

class Interpreter;
class Variable;
class Func;
class DynamicFunc;

using namespace std;

//using std::any = variant<shared_ptr<Variable>, shared_ptr<Func>, shared_ptr<DynamicFunc>, TYPE, Array<TYPE>>;

// Ident definition

class Variable {
public:
	String name;
	std::any value;
	Variable() {};
	Variable(String n, std::any v) : name(n), value(v) {};

	String toString() {
		String s;
		if (auto p = std::any_cast<TYPE>(&value)) {
			std::visit([&s](auto& x) {
				s = Format();
			}, *p);
		}
		else if (auto p = std::any_cast<shared_ptr<Variable>>(&value)) {
			s = U"Variable";
		}
		else {
			s = U"Function";
		}
		return s;
	}
};

class Func {
public:
	String name;
	Func() {};
	Func(String n) : name(n) {};
	virtual std::any invoke(Array<std::any> args) { return mono{}; }; //return Variable
};

class DynamicFunc : public Func {
public:
	shared_ptr<Interpreter> context;
	Array<shared_ptr<Token>> params;
	Array<shared_ptr<Token>> block;
	DynamicFunc(shared_ptr<Interpreter> c, String n, Array<shared_ptr<Token>> p, Array<shared_ptr<Token>> b) : Func(n), context(c), params(p), block(b) {};
	std::any invoke(Array<std::any> args) override;
};

class Println : public Func {
public:
	Println() : Func(U"println") {};
	std::any invoke(Array<std::any> args) override;
};

class Scope {
public:
	shared_ptr<Scope> parent;
	HashTable<String, shared_ptr<Func>> functions;
	HashTable<String, shared_ptr<Variable>> variables;

	Scope() : functions({}), variables({}) {};
	Scope(HashTable<String, shared_ptr<Variable>> vs) : functions({}), variables(vs) {};
};


// Interpreter definition

class Interpreter {
public:
	Array<shared_ptr<Token>> body;
	shared_ptr<Scope> grobal;
	shared_ptr<Scope> local;

	Interpreter(){};
	Interpreter& init(Array<shared_ptr<Token>> b, HashTable<String, shared_ptr<Variable>> vs);
	HashTable<String, shared_ptr<Variable>> run();
	std::any process(Array<shared_ptr<Token>> b, Optional<bool>& ret, Optional<bool>& brk);
	std::any expression(shared_ptr<Token> expr);
	int32 digit(shared_ptr<Token> token);
	std::any ident(shared_ptr<Token> token);
	shared_ptr<Variable> assign(shared_ptr<Token> expr);
	shared_ptr<Variable> variable(std::any value);
	std::any value(std::any v);
	int32 integer(std::any v);
	String str(std::any v);
	std::any calc(shared_ptr<Token> expr);
	int32 calcInt(String sign, int32 left, int32 right);
	std::any calcString(String sign, String left, String right);
	std::any unaryCalc(shared_ptr<Token> expr);
	int32 unaryCalcInt(String sign, int32 left);
	int32 unaryCalcString(String sign, String left);
	std::any invoke(shared_ptr<Token> expr);
	shared_ptr<Func> func(std::any v);
	std::any func(shared_ptr<Token> token);
	std::any if_(shared_ptr<Token> token, Optional<bool>& ret, Optional<bool>& brk);
	bool isTrue(shared_ptr<Token> token);
	bool isTrue(std::any v);
	int32 toInt(bool b);
	std::any while_(shared_ptr<Token> token, Optional<bool>& ret);
	std::any var(shared_ptr<Token> token);
	shared_ptr<Variable> newVariable(String name);
	shared_ptr<DynamicFunc> fexpr(shared_ptr<Token> token);
	std::any str(shared_ptr<Token> token);
	std::any blank(shared_ptr<Token> token);
	std::any newArray(shared_ptr<Token> expr);
	std::any accessArray(shared_ptr<Token> expr);
	std::any arr(std::any v);
};

