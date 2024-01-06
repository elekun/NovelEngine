#include "stdafx.h"
#include "MyError.hpp"

StringView ArgumentError::type() const noexcept {
    return StringView( U"ArgumentError" );
}

StringView ArgumentParseError::type() const noexcept {
	return StringView(U"ArgumentError");
}

void ThrowParseError(const String type, const v_type value) {
	String mes = std::visit([type](auto& x) {
		return U"{}({}型)を{}型に変換することはできません"_fmt(x, Unicode::Widen(typeid(x).name()), type);
	}, value);
	throw ArgumentParseError{ mes };
}

void ThrowArgumentError(const StringView operation, const StringView name, const String type, const v_type value) {
	String mes = std::visit([operation, name, type](auto& x) {
		return U"{}命令の引数{}は{}型です\n{}は{}型です"_fmt(operation, name, type, x, Unicode::Widen(typeid(x).name()));
	}, value);
	throw ArgumentError{ mes };
}

