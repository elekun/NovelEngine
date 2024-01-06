#include "stdafx.h"
#include "Token.hpp"

String Token::toString() {
	return U"{}\"{}\""_fmt(kind, value);
}
String Token::paren() {
	if (left == nullptr && right == nullptr) {
		return value;
	}
	else {
		String s = U"(";
		s += left ? left->paren() : U"";
		s += value;
		s += right ? right->paren() : U"";
		s += U")";
		return s;
	}
}

/*
* Aexp : arithmetic expression
* a ::= n | X | a_0+a_1 | a_0*a_1
* Bexp : boolean expression
* b ::= true | false | a_0<=a_1 | !b | b_0 && b_1
* Comm : Command
* c ::= skip | X::=a | c_0;c_1 | if b then c_0 else c_1 | while b do c
*/
