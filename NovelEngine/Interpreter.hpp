#pragma once
#include "GameScene.hpp"
#include "Token.hpp"
#include <any>

class Interpreter;
class Variable;
class Func;
class DynamicFunc;

using namespace std;

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
	String getPrintString(std::any arg);
};

class Size_ : public Func {
public:
	Size_() : Func(U"size") {};
	std::any invoke(Array<std::any> args) override;
};

class Length : public Func {
public:
	Length() : Func(U"length") {};
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
	std::any getValue();
	std::any process(Array<shared_ptr<Token>> b, Optional<bool>& ret, Optional<bool>& brk);
	std::any expression(shared_ptr<Token> expr);
	int32 digit(shared_ptr<Token> token);
	double decimal(shared_ptr<Token> token);
	std::any ident(shared_ptr<Token> token);
	shared_ptr<Variable> assign(shared_ptr<Token> expr);
	shared_ptr<Variable> checkArray(shared_ptr<Token> expr, Array<int32>& indexList);
	shared_ptr<Variable> variable(std::any value);
	std::any value(std::any v);
	int32 integer(std::any v);
	String str(std::any v);
	std::any calc(shared_ptr<Token> expr);
	int32 calcInt(String sign, int32 left, int32 right);
	std::any calcDecimal(String sign, double left, double right);
	std::any calcString(String sign, String left, String right);
	std::any unaryCalc(shared_ptr<Token> expr);
	int32 unaryCalcInt(String sign, int32 left);
	double unaryCalcDecimal(String sign, double left);
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
	std::any for_(shared_ptr<Token> token, Optional<bool>& ret);
};

