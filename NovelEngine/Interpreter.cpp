#include "stdafx.h"
#include "Interpreter.hpp"


std::any DynamicFunc::invoke(Array<std::any> args) {
	shared_ptr<Scope> parent = context->local;
	context->local = make_shared<Scope>(Scope{});
	context->local->parent = parent;


	for (auto [i, param] : Indexed(params)) {
		shared_ptr<Variable> v = context->newVariable(param->value);
		if (i < args.size()) {
			v->value = context->value(args[i]);
		}
		else {
			v->value = mono{};
		}
	}
	Optional<bool> ret = false;
	Optional<bool> brk;
	std::any val = context->process(block, ret, brk);
	context->local = parent;
	return val;
}

std::any Println::invoke(Array<std::any> args) {
	String s = U"";
	for (int32 i = 0; i < args.size(); i++) {
		s += getPrintString(args[i]);
		if (i < args.size() - 1) {
			s += U" ";
		}
	}
	Console << s;
	return mono{};
}

String Println::getPrintString(std::any arg) {
	if (auto p = std::any_cast<int32>(&arg)) {
		return Format(*p);
	}
	else if (auto p = std::any_cast<double>(&arg)) {
		return Format(*p);
	}
	else if (auto p = std::any_cast<String>(&arg)) {
		return *p;
	}
	else if (auto p = std::any_cast<Array<std::any>>(&arg)) {
		String s = U"{";
		for (int32 i = 0; i < p->size(); i++) {
			s += getPrintString(p->at(i));
			if (i < p->size() - 1) {
				s += U",";
			}
		}
		s += U"}";
		return s;
	}
	else {
		return U"Function";
	}
}

std::any Size_::invoke(Array<std::any> args) {
	if (args.size() != 1) {
		throw Error{U"size() needs 1 argument."};
	}

	std::any arg = args[0];
	if (auto p = std::any_cast<Array<std::any>>(&arg)) {
		return (int32)p->size();
	}
	else {
		return 1;
	}
}

std::any Length::invoke(Array<std::any> args) {
	if (args.size() != 1) {
		throw Error{U"length() needs 1 argument."};
	}

	std::any arg = args[0];
	if (auto p = std::any_cast<String>(&arg)) {
		return (int32)p->length();
	}
	else {
		throw Error{U"length() needs string argument."};
	}
}

Interpreter& Interpreter::init(Array<shared_ptr<Token>> b, HashTable<String, shared_ptr<Variable>> vs) {
	grobal = make_shared<Scope>(Scope{ vs });
	local = grobal;
	body = b;

	Array<shared_ptr<Func>> tmp_func = {
		make_shared<Println>(Println()),
		make_shared<Size_>(Size_()),
		make_shared<Length>(Length())
	};
	for (auto f : tmp_func) {
		grobal->functions.emplace(f->name, f);
	}
	
	return *this;
}

HashTable<String, shared_ptr<Variable>> Interpreter::run() {
	Optional<bool> ret;
	Optional<bool> brk;
	process(body, ret, brk);
	return grobal->variables;
}

std::any Interpreter::process(Array<shared_ptr<Token>> b, Optional<bool>& ret, Optional<bool>& brk) {
	for (auto expr : b) {
		if (expr->kind == U"if") {
			std::any val = if_(expr, ret, brk);
			if (ret.has_value() && ret.value()) {
				return val;
			}
		}
		else if (expr->kind == U"ret") {
			if (!ret.has_value()) {
				throw Error{U"Can not return"};
			}
			ret = true;
			if (expr->left) {
				return expression(expr->left);
			}
			else {
				return mono{};
			}
		}
		else if (expr->kind == U"while") {
			std::any val = while_(expr, ret);
			if (ret.has_value() && ret.value()) {
				return val;
			}
		}
		else if (expr->kind == U"brk") {
			if (!brk.has_value()) {
				throw Error{U"Can not break"};
			}
			brk = true;
			return mono{};
		}
		else if(expr->kind == U"var") {
			var(expr);
		}
		else {
			expression(expr);
		}
	}
	return mono{};
}

std::any Interpreter::expression(shared_ptr<Token> expr) {
	if (expr->kind == U"digit") {
		return digit(expr);
	}
	if (expr->kind == U"decimal") {
		return decimal(expr);
	}
	else if (expr->kind == U"string") {
		return str(expr);
	}
	else if (expr->kind == U"ident") {
		return ident(expr);
	}
	else if (expr->kind == U"blank") {
		return blank(expr);
	}
	else if (expr->kind == U"newArray") {
		return newArray(expr);
	}
	else if (expr->kind == U"bracket") {
		return accessArray(expr);
	}
	else if (expr->kind == U"func") {
		return func(expr);
	}
	else if (expr->kind == U"fexpr") {
		return fexpr(expr);
	}
	else if (expr->kind == U"paren") {
		return invoke(expr);
	}
	else if (expr->kind == U"sign" && expr->value == U"=") {
		return assign(expr);
	}
	else if (expr->kind == U"unary") {
		return unaryCalc(expr);
	}
	else if (expr->kind == U"sign") {
		return calc(expr);
	}
	else {
		throw Error{U"Expression error"};
	}
}

int32 Interpreter::digit(shared_ptr<Token> token) {
	return Parse<int32>(token->value);
}

double Interpreter::decimal(shared_ptr<Token> token) {
	return Parse<double>(token->value);
}

std::any Interpreter::ident(shared_ptr<Token> token) {
	String name = token->value;
	shared_ptr<Scope> scope = local;
	while (scope != nullptr) {
		if (scope->functions.contains(name)) {
			return scope->functions[name];
		}
		if (scope->variables.contains(name)) {
			return scope->variables[name];
		}
		scope = scope->parent;
	}
	return newVariable(name);
}

shared_ptr<Variable> Interpreter::assign(shared_ptr<Token> expr) {
	shared_ptr<Variable> var = variable(expression(expr->left));
	std::any v = value(expression(expr->right));
	var->value = v;
	return var;
}

shared_ptr<Variable> Interpreter::variable(std::any value) {
	if (auto p = std::any_cast<shared_ptr<Variable>>(value)) {
		return p;
	}
	else {
		throw Error{U"left value error"};
	}
}

std::any Interpreter::value(std::any v) {
	if (auto p = std::any_cast<int32>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<double>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<String>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<Array<std::any>>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<shared_ptr<Variable>>(&v)) {
		return (*p)->value;
	}
	else if (auto p = std::any_cast<shared_ptr<DynamicFunc>>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<mono>(&v)) {
		return *p;
	}
	else {
		throw Error{U"right value error"};
	}
}

int32 Interpreter::integer(std::any v) {
	if (auto p = std::any_cast<int32>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<String>(&v)) {
		return Parse<int32>(*p);
	}
	else if (auto p = std::any_cast<shared_ptr<Variable>>(&v)) {
		return integer((*p)->value);
	}
	throw Error{U"right value error"};
}

String Interpreter::str(std::any v) {
	if (auto p = std::any_cast<int32>(&v)) {
		return Format(*p);
	}
	if (auto p = std::any_cast<double>(&v)) {
		return Format(*p);
	}
	if (auto p = std::any_cast<String>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<shared_ptr<Variable>>(&v)) {
		return str((*p)->value);
	}
	throw Error{U"right value error"};
}

std::any Interpreter::calc(shared_ptr<Token> expr) {
	auto l = value(expression(expr->left));
	auto r = value(expression(expr->right));

	if (auto left = std::any_cast<int32>(&l)) {
		if (auto right = std::any_cast<int32>(&r)) {
			return calcInt(expr->value, *left, *right);
		}
		else if (auto right = std::any_cast<double>(&r)) {
			return calcDecimal(expr->value, *left, *right);
		}
	}
	else if (auto left = std::any_cast<double>(&l)) {
		if (auto right = std::any_cast<int32>(&r)) {
			return calcDecimal(expr->value, *left, *right);
		}
		else if (auto right = std::any_cast<double>(&r)) {
			return calcDecimal(expr->value, *left, *right);
		}
	}

	return calcString(expr->value, str(l), str(r));
}

int32 Interpreter::calcInt(String sign, int32 left, int32 right) {
	if (sign == U"+") {
		return left + right;
	}
	else if (sign == U"-") {
		return left - right;
	}
	else if (sign == U"*") {
		return left * right;
	}
	else if (sign == U"/") {
		return left / right;
	}
	else if (sign == U"==") {
		return toInt(left == right);
	}
	else if (sign == U"!=") {
		return toInt(left != right);
	}
	else if (sign == U"<") {
		return toInt(left < right);
	}
	else if (sign == U"<=") {
		return toInt(left <= right);
	}
	else if (sign == U">") {
		return toInt(left > right);
	}
	else if (sign == U">=") {
		return toInt(left >= right);
	}
	else if (sign == U"&&") {
		return toInt(isTrue(left) && isTrue(right));
	}
	else if (sign == U"||") {
		return toInt(isTrue(left) || isTrue(right));
	}
	else {
		throw Error{U"Unknown sign for Calc"};
	}
}

std::any Interpreter::calcDecimal(String sign, double left, double right) {
	if (sign == U"+") {
		return left + right;
	}
	else if (sign == U"-") {
		return left - right;
	}
	else if (sign == U"*") {
		return left * right;
	}
	else if (sign == U"/") {
		return left / right;
	}
	else if (sign == U"==") {
		return toInt(left == right);
	}
	else if (sign == U"!=") {
		return toInt(left != right);
	}
	else if (sign == U"<") {
		return toInt(left < right);
	}
	else if (sign == U"<=") {
		return toInt(left <= right);
	}
	else if (sign == U">") {
		return toInt(left > right);
	}
	else if (sign == U">=") {
		return toInt(left >= right);
	}
	else {
		throw Error{U"Unknown sign for Calc"};
	}
}

std::any Interpreter::calcString(String sign, String left, String right) {
	if (sign == U"+") {
		return left + right;
	}
	else if (sign == U"==") {
		return toInt(left == right);
	}
	else if (sign == U"!=") {
		return toInt(left != right);
	}
	else if (sign == U"&&") {
		return toInt(isTrue(left) && isTrue(right));
	}
	else if (sign == U"||") {
		return toInt(isTrue(left) || isTrue(right));
	}
	else {
		throw Error{U"Unknown sign for Calc"};
	}
}

std::any Interpreter::unaryCalc(shared_ptr<Token> expr) {
	auto v = value(expression(expr->left));
	if (auto p = std::any_cast<int32>(&v)) {
		return unaryCalcInt(expr->value, *p);
	}
	else if (auto p = std::any_cast<String>(&v)) {
		return unaryCalcString(expr->value, *p);
	}
	else {
		throw Error{U"unaryCalc error"};
	}
}

int32 Interpreter::unaryCalcInt(String sign, int32 left) {
	if (sign == U"+") {
		return left;
	}
	else if (sign == U"-") {
		return -left;
	}
	else if (sign == U"!") {
		return toInt(!isTrue(left));
	}
	else {
		throw Error{U"unaryCalcInt Error"};
	}
}

double Interpreter::unaryCalcDecimal(String sign, double left) {
	if (sign == U"+") {
		return left;
	}
	else if (sign == U"-") {
		return -left;
	}
	else {
		throw Error{U"unaryCalcDecimal Error"};
	}
}

int32 Interpreter::unaryCalcString(String sign, String left) {
	if (sign == U"!") {
		return toInt(!isTrue(left));
	}
	else {
		throw Error{U"unaryCalcString Error"};
	}
}

std::any Interpreter::invoke(shared_ptr<Token> expr) {
	shared_ptr<Func> f = func(expression(expr->left));
	Array<std::any> values = {};
	for (auto arg : expr->params) {
		values << value(expression(arg));
	}
	return f->invoke(values);
}

shared_ptr<Func> Interpreter::func(std::any v) {
	if (auto p = std::any_cast<shared_ptr<Func>>(&v)) {
		return *p;
	}
	if (auto p = std::any_cast<shared_ptr<DynamicFunc>>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<shared_ptr<Variable>>(&v)) {
		return func((*p)->value);
	}
	else {
		throw Error{U"Not a function"};
	}
}

std::any Interpreter::func(shared_ptr<Token> token) {
	String name = token->ident->value;
	if (local->functions.contains(name)) {
		throw Error{U"Name was used"};
	}
	if (local->variables.contains(name)) {
		throw Error{U"Name was used"};
	}

	Array<String> paramCheckList = {};
	for (auto p : token->params) {
		String param = p->value;
		if (paramCheckList.contains(param)) {
			throw Error{U"Parameter name was used"};
		}
		paramCheckList << param;
	}

	shared_ptr<Interpreter> interpreter = make_shared<Interpreter>(Interpreter{});
	interpreter->grobal = this->grobal;
	interpreter->local = this->local;
	interpreter->body = this->body;
	DynamicFunc func{ interpreter, U"", token->params, token->block };
	local->functions.emplace(name, make_shared<DynamicFunc>(func));

	return mono{};
}

std::any Interpreter::if_(shared_ptr<Token> token, Optional<bool>& ret, Optional<bool>& brk) {
	Array<shared_ptr<Token>> block;
	if (isTrue(token->left)) {
		block = token->block;
	}
	else {
		block = token->blockOfElse;
	}

	if (!block.isEmpty()) {
		return process(block, ret, brk);
	}
	else {
		return mono{};
	}
}

bool Interpreter::isTrue(shared_ptr<Token> token) {
	std::any v = value(expression(token));
	bool f = false;
	if (auto p = std::any_cast<int32>(&v)) {
		if (*p != 0) {
			f = true;
		}
	}
	return f;
}

bool Interpreter::isTrue(std::any v) {
	if (auto p = std::any_cast<int32>(&v)) {
		return 0 != *p;
	}
	else if (auto p = std::any_cast<double>(&v)) {
		return 0.0 != *p;
	}
	else if (auto p = std::any_cast<String>(&v)) {
		return U"" != *p;
	}
	else if (auto p = std::any_cast<shared_ptr<DynamicFunc>>(&v)) {
		return true;
	}
	else {
		return false;
	}
}

int32 Interpreter::toInt(bool b) { return b ? 1 : 0; }

std::any Interpreter::while_(shared_ptr<Token> token, Optional<bool>& ret) {
	Optional<bool> brk = false;
	std::any val;
	while (isTrue(token->left)) {
		val = process(token->block, ret, brk);
		if (ret.has_value() && ret.value()) {
			return val;
		}
		if (brk.value()) {
			return mono{};
		}
	}
	return mono{};
}

std::any Interpreter::var(shared_ptr<Token> token) {
	for (auto item : token->block) {
		String name;
		shared_ptr<Token> expr;
		if (item->kind == U"ident") {
			name = item->value;
			expr = nullptr;
		}
		else if (item->kind == U"sign" && item->value == U"=") {
			name = item->left->value;
			expr = item;
		}
		else {
			throw Error{U"var error"};
		}

		if (!local->variables.contains(name)) {
			newVariable(name);
		}
		if (expr != nullptr) {
			expression(expr);
		}
	}
	return mono{};
}

shared_ptr<Variable> Interpreter::newVariable(String name) {
	shared_ptr<Variable> v = make_shared<Variable>(Variable{ name, 0 });
	local->variables.emplace(name, v);
	return v;
}

shared_ptr<DynamicFunc> Interpreter::fexpr(shared_ptr<Token> token) {
	Array<String> paramCheckList = {};
	for (auto param : token->params) {
		if (paramCheckList.contains(param->value)) {
			throw Error{U"Parameter name was used"};
		}
		paramCheckList << param->value;
	}

	shared_ptr<Interpreter> interpreter = make_shared<Interpreter>(Interpreter{});
	interpreter->grobal = this->grobal;
	interpreter->local = this->local;
	interpreter->body = this->body;
	DynamicFunc func{ interpreter, U"", token->params, token->block };

	return make_shared<DynamicFunc>(func);
}

std::any Interpreter::str(shared_ptr<Token> token) {
	return token->value;
}

std::any Interpreter::blank(shared_ptr<Token> token) {
	return mono{};
}

std::any Interpreter::newArray(shared_ptr<Token> expr) {
	Array<std::any> a = {};
	for (auto item : expr->params) {
		a << value(expression(item));
	}
	return a;
}

std::any Interpreter::accessArray(shared_ptr<Token> expr) {
	std::any ar = arr(expression(expr->left));
	int32 index = integer(expression(expr->right));

	if (auto p = std::any_cast<Array<std::any>>(&ar)) {
		return (*p)[index];
	}
	if (auto p = std::any_cast<String>(&ar)) {
		return String{ p->at(index) };
	}
	throw Error{U"array access error"};
}

std::any Interpreter::arr(std::any v) {
	if (auto p = std::any_cast<Array<std::any>>(&v)) {
		return *p;
	}
	if (auto p = std::any_cast<String>(&v)) {
		return *p;
	}
	else if (auto p = std::any_cast<shared_ptr<Variable>>(&v)) {
		return arr((*p)->value);
	}
	throw Error{U"right value error"};
}
