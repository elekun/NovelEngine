#include "stdafx.h"
#include "Interpreter.hpp"

r_type Println::invoke(Array<r_type> args) {
	for (int32 i = 0; i < args.size(); i++) {
		if (auto p = get_if<TYPE>(&args[i])) {
			std::visit([](auto& x) {
				Console << x;
			}, *p);
		}
		else if (auto p = get_if<shared_ptr<Variable>>(&args[i])) {
			Console << U"Variable";
		}
		else {
			Console << U"Function";
		}

		if (i < args.size() - 1) {
			Console << U" ";
		}
	}
	return mono{};
}

r_type DynamicFunc::invoke(Array<r_type> args) {
	shared_ptr<Scope> parent = context->local;
	context->local = make_shared<Scope>(Scope{});
	context->local->parent = parent;


	for (auto[i, param]: Indexed(params)) {
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
	r_type val = context->process(block, ret, brk);
	context->local = parent;
	return val;
}



Interpreter& Interpreter::init(Array<shared_ptr<Token>> b, HashTable<String, shared_ptr<Variable>> vs) {
	grobal = make_shared<Scope>(Scope{ vs });
	local = grobal;
	body = b;

	Array<shared_ptr<Func>> tmp_func = {
		make_shared<Println>(Println())
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

r_type Interpreter::process(Array<shared_ptr<Token>> b, Optional<bool>& ret, Optional<bool>& brk) {
	for (auto expr : b) {
		if (expr->kind == U"if") {
			r_type val = if_(expr, ret, brk);
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
			r_type val = while_(expr, ret);
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

r_type Interpreter::expression(shared_ptr<Token> expr) {
	if (expr->kind == U"digit") {
		return digit(expr); // return TYPE
	}
	else if (expr->kind == U"string") {
		return str(expr);
	}
	else if (expr->kind == U"ident") {
		return ident(expr); // return Variable or Func
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
		return func(expr); // return nullptr?
	}
	else if (expr->kind == U"fexpr") {
		return fexpr(expr);
	}
	else if (expr->kind == U"paren") {
		return invoke(expr); // return function_return(TYPE)
	}
	else if (expr->kind == U"sign" && expr->value == U"=") {
		return assign(expr); // return Variable
	}
	else if (expr->kind == U"unary") {
		return unaryCalc(expr); // return TYPE
	}
	else if (expr->kind == U"sign") {
		return calc(expr); // return TYPE
	}
	else {
		throw Error{U"Expression error"};
	}
}

int32 Interpreter::digit(shared_ptr<Token> token) {
	return Parse<int32>(token->value);
}

r_type Interpreter::ident(shared_ptr<Token> token) {
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
	r_type v = value(expression(expr->right));
	var->value = v;
	return var;
}

shared_ptr<Variable> Interpreter::variable(r_type value) {
	if (auto p = get_if<shared_ptr<Variable>>(&value)) {
		return *p;
	}
	else {
		throw Error{U"left value error"};
	}
}

r_type Interpreter::value(r_type v) {
	if (auto p = get_if<TYPE>(&v)) {
		return *p;
	}
	else if (auto p = get_if<Array<TYPE>>(&v)) {
		return *p;
	}
	else if (auto p = get_if<shared_ptr<DynamicFunc>>(&v)) {
		return *p;
	}
	else if (auto p = get_if<shared_ptr<Variable>>(&v)) {
		return (*p)->value;
	}
	else {
		throw Error{U"right value error"};
	}
}

TYPE Interpreter::checkTYPEValue(r_type v) {
	if (auto p = get_if<TYPE>(&v)) {
		return *p;
	}
	else {
		throw Error{U"value error"};
	}
}

int32 Interpreter::integer(r_type v) {
	if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<int32>(p)) {
			return *q;
		}
		if (auto q = get_if<String>(p)) {
			return Parse<int32>(*q);
		}
		throw Error{U"right value error"};
	}
	else if (auto p = get_if<shared_ptr<Variable>>(&v)) {
		return integer((*p)->value);
	}
	throw Error{U"right value error"};
}

String Interpreter::str(r_type v) {
	if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<int32>(p)) {
			return Format(*q);
		}
		if (auto q = get_if<String>(p)) {
			return *q;
		}
		throw Error{U"right value error"};
	}
	else if (auto p = get_if<shared_ptr<Variable>>(&v)) {
		return str((*p)->value);
	}
	throw Error{U"right value error"};
}

TYPE Interpreter::calc(shared_ptr<Token> expr) {
	auto l = checkTYPEValue(value(expression(expr->left)));
	auto r = checkTYPEValue(value(expression(expr->right)));

	if (auto left = get_if<int32>(&l)) {
		if (auto right = get_if<int32>(&r)) {
			return calcInt(expr->value, *left, *right);
		}
	}
	return calcString(expr->value, str(l), str(r));
}

TYPE Interpreter::calcInt(String sign, int32 left, int32 right) {
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

TYPE Interpreter::calcString(String sign, String left, String right) {
	if (sign == U"+") {
		return left + right;
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

TYPE Interpreter::unaryCalc(shared_ptr<Token> expr) {
	auto v = value(expression(expr->left));
	if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<int32>(p)) {
			return unaryCalcInt(expr->value, *q);
		}
		else if (auto q = get_if<String>(p)) {
			return unaryCalcString(expr->value, *q);
		}
		else {
			throw Error{U"unaryCalc error"};
		}
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

int32 Interpreter::unaryCalcString(String sign, String left) {
	if (sign == U"!") {
		return toInt(!isTrue(left));
	}
	else {
		throw Error{U"unaryCalcString Error"};
	}
}

r_type Interpreter::invoke(shared_ptr<Token> expr) {
	shared_ptr<Func> f = func(expression(expr->left));
	Array<r_type> values = {};
	for (auto arg : expr->params) {
		values << value(expression(arg));
	}
	return f->invoke(values);
}

shared_ptr<Func> Interpreter::func(r_type v) {
	if (auto p = get_if<shared_ptr<Func>>(&v)) {
		return *p;
	}
	if (auto p = get_if<shared_ptr<DynamicFunc>>(&v)) {
		return *p;
	}
	else if (auto p = get_if<shared_ptr<Variable>>(&v)) {
		return func((*p)->value);
	}
	else {
		throw Error{U"Not a function"};
	}
}

TYPE Interpreter::func(shared_ptr<Token> token) {
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

r_type Interpreter::if_(shared_ptr<Token> token, Optional<bool>& ret, Optional<bool>& brk) {
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
	r_type v = value(expression(token));
	bool f = false;
	if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<int32>(p)) {
			if (*q != 0) {
				f = true;
			}
		}
	}
	return f;
}

bool Interpreter::isTrue(r_type v) {
	if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<int32>(p)) {
			return 0 != *q;
		}
		else if (auto q = get_if<String>(p)) {
			return U"" != *q;
		}
		else {
			return false;
		}
	}
	else if (auto p = get_if<shared_ptr<DynamicFunc>>(&v)) {
		return true;
	}
	else {
		return false;
	}
}

int32 Interpreter::toInt(bool b) { return b ? 1 : 0; }

r_type Interpreter::while_(shared_ptr<Token> token, Optional<bool>& ret) {
	Optional<bool> brk = false;
	r_type val;
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

r_type Interpreter::var(shared_ptr<Token> token) {
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

r_type Interpreter::str(shared_ptr<Token> token) {
	return token->value;
}

r_type Interpreter::blank(shared_ptr<Token> token) {
	return mono{};
}

r_type Interpreter::newArray(shared_ptr<Token> expr) {
	Array<TYPE> a = {};
	for (auto item : expr->params) {
		a << checkTYPEValue(value(expression(item)));
	}
	return a;
}

r_type Interpreter::accessArray(shared_ptr<Token> expr) {
	r_type ar = arr(expression(expr->left));
	int32 index = integer(expression(expr->right));

	if (auto p = get_if<Array<TYPE>>(&ar)) {
		return (*p)[index];
	}
	else if (auto p = get_if<TYPE>(&ar)) {
		if (auto q = get_if<String>(p)) {
			return String{ q->at(index) };
		}
	}
	throw Error{U"array access error"};
}

r_type Interpreter::arr(r_type v) {
	if (auto p = get_if<Array<TYPE>>(&v)) {
		return *p;
	}
	else if (auto p = get_if<TYPE>(&v)) {
		if (auto q = get_if<String>(p)) {
			return *q;
		}
	}
	else if (auto p = get_if<shared_ptr<Variable>>(&v)) {
		return arr((*p)->value);
	}
	throw Error{U"right value error"};
}


