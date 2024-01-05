#pragma once
#include <memory>
#include "GameScene.hpp"
#include "Token.hpp"

class Interpreter;
class Variable;
class Func;
class DynamicFunc;

using namespace std;

using r_type = variant<shared_ptr<Variable>, shared_ptr<Func>, shared_ptr<DynamicFunc>, TYPE>;

// Ident definition

class Variable {
public:
	String name;
	r_type value;
	Variable() {};
	Variable(String n, r_type v) : name(n), value(v) {};

	String toString() {
		String s;
		if (auto p = get_if<TYPE>(&value)) {
			std::visit([&s](auto& x) {
				s = Format();
			}, *p);
		}
		else if (auto p = get_if<shared_ptr<Variable>>(&value)) {
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
	virtual r_type invoke(Array<r_type> args) { return mono{}; }; //return Variable
};

class DynamicFunc : public Func {
public:
	shared_ptr<Interpreter> context;
	Array<shared_ptr<Token>> params;
	Array<shared_ptr<Token>> block;
	DynamicFunc(shared_ptr<Interpreter> c, String n, Array<shared_ptr<Token>> p, Array<shared_ptr<Token>> b) : Func(n), context(c), params(p), block(b) {};
	r_type invoke(Array<r_type> args) override;
};

class Println : public Func {
public:
	Println() : Func(U"println") {};
	r_type invoke(Array<r_type> args) override;
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
	r_type process(Array<shared_ptr<Token>> b, Optional<bool>& ret, Optional<bool>& brk);
	r_type expression(shared_ptr<Token> expr);
	int32 digit(shared_ptr<Token> token);
	r_type ident(shared_ptr<Token> token);
	shared_ptr<Variable> assign(shared_ptr<Token> expr);
	shared_ptr<Variable> variable(r_type value);
	r_type value(r_type v);
	TYPE checkTYPEValue(r_type v);
	int32 integer(r_type v);
	String str(r_type v);
	TYPE calc(shared_ptr<Token> expr);
	TYPE calcInt(String sign, int32 left, int32 right);
	TYPE calcString(String sign, String left, String right);
	TYPE unaryCalc(shared_ptr<Token> expr);
	int32 unaryCalcInt(String sign, int32 left);
	int32 unaryCalcString(String sign, String left);
	r_type invoke(shared_ptr<Token> expr);
	shared_ptr<Func> func(r_type v);
	TYPE func(shared_ptr<Token> token);
	r_type if_(shared_ptr<Token> token, Optional<bool>& ret, Optional<bool>& brk);
	bool isTrue(shared_ptr<Token> token);
	bool isTrue(r_type v);
	int32 toInt(bool b);
	r_type while_(shared_ptr<Token> token, Optional<bool>& ret);
	r_type var(shared_ptr<Token> token);
	shared_ptr<Variable> newVariable(String name);
	shared_ptr<DynamicFunc> fexpr(shared_ptr<Token> token);
	r_type str(shared_ptr<Token> token);
};

