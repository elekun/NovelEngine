#include "stdafx.h"
#include "GameScene.hpp"

void ThrowParseError(const String type, const v_type value) {
	String mes = std::visit([type](auto& x) {
		return U"{}({}型)を{}型に変換することはできません"_fmt(x, Unicode::Widen(typeid(x).name()), type);
	}, value);
	throw Error{ mes };
}

void ThrowArgumentError(const StringView operation, const StringView name, const String type, const v_type value) {
	String mes = std::visit([operation, name, type](auto& x) {
		return U"{}命令の引数{}は{}型です\n{}は{}型です"_fmt(operation, name, type, x, Unicode::Widen(typeid(x).name()));
	}, value);
	throw Error{ mes };
}
